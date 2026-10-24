// main.cpp - the single translation unit. Own file includes only, in order.
#include <windows.h>
#include "base.cpp"
#include "raster.cpp"

static BOOL Running = TRUE;
static HWND MainWindow = 0;
static HDC WindowDC = 0;
static BITMAPINFO ScreenBitmapInfo;
static BOOL DumpMode = FALSE;
static BOOL NoCapMode = FALSE; // --nocap: run flat out for benchmarks only

// The visual-analysis tool lives at the bottom of this file, after the engine code.
// Declared here so WinMain can start it and tick it. Without --dump it never runs.
void DumpBegin(LPSTR commandLine);
void DumpTick(HWND Window);

// Used by WM_PAINT, which is defined before PresentBackbuffer's body.
void PresentBackbuffer(HWND Window);

// Tiny command-line flag search, so both --dump and --nocap use the same rule.
bool CommandLineHas(LPSTR commandLine, const char* flag)
{
    if (commandLine == 0 || flag == 0 || flag[0] == 0) return false;

    i32 flagLength = 0;
    while (flag[flagLength] != 0) ++flagLength;

    for (i32 i = 0; commandLine[i] != 0; ++i)
    {
        i32 j = 0;
        while (j < flagLength && commandLine[i + j] == flag[j]) ++j;
        if (j == flagLength) return true;
    }

    return false;
}

// --- Window messages ---
LRESULT CALLBACK WindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
        case WM_CLOSE:
        {
            DestroyWindow(Window);
        } break;

        case WM_DESTROY:
        {
            // Later steps save the world here; it must run once, however the window closes.
            ReleaseMouseCapture();

            // CS_OWNDC: this is the window's one device context, released once here.
            if (WindowDC)
            {
                ReleaseDC(Window, WindowDC);
                WindowDC = 0;
            }
            Running = FALSE;
            PostQuitMessage(0);
        } break;

        case WM_ACTIVATEAPP:
        {
            // Alt+Tab and other app switches give the cursor back.
            if (!WParam)
            {
                ReleaseMouseCapture();
            }
        } break;

        case WM_ERASEBKGND:
        {
            // The backbuffer covers every pixel, so there is nothing to erase.
            Result = 1;
        } break;

        case WM_PAINT:
        {
            // Windows asks for a repaint on resize, restore and expose. Without this the
            // client area can stay blank until the next frame.
            PAINTSTRUCT Paint;
            HDC PaintDC = BeginPaint(Window, &Paint);
            if (PaintDC)
            {
                PresentBackbuffer(Window);
            }
            EndPaint(Window, &Paint);
        } break;

        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }

    return Result;
}

// --- Step 7: camera turning and the 3D test scene ---
const f32 CAMERA_TURN_SPEED = 1.5f; // radians per second

void UpdateCameraWithArrowKeys()
{
    if (In.left) Cam.yaw -= CAMERA_TURN_SPEED * Dt;
    if (In.right) Cam.yaw += CAMERA_TURN_SPEED * Dt;
    if (In.up) Cam.pitch += CAMERA_TURN_SPEED * Dt;
    if (In.down) Cam.pitch -= CAMERA_TURN_SPEED * Dt;

    Cam.pitch = Clamp(Cam.pitch, -1.5f, 1.5f);

    DebugSet(1, "YAW", i32(Cam.yaw * 1000.0f));
}

void DrawTestTriangles3D()
{
    DrawTriangle3D(V3(-2.0f, 0.0f, 0.0f), V3(2.0f, 0.0f, 0.0f), V3(0.0f, 3.0f, 0.0f),
                   MakeColor(255, 160, 0));
    DrawTriangle3D(V3(-1.0f, 0.0f, -1.0f), V3(1.0f, 0.0f, 1.0f), V3(0.0f, 2.0f, 0.0f),
                   MakeColor(0, 200, 200));
}

void PresentBackbuffer(HWND Window)
{
    // The CS_OWNDC device context is held from window creation until WM_DESTROY,
    // so no HDC is created or freed per frame.
    if (WindowDC == 0) return;

    // WM_PAINT can arrive during CreateWindowExA, before WinMain fills this in.
    if (ScreenBitmapInfo.bmiHeader.biBitCount == 0) return;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    i32 ClientWidth = ClientRect.right - ClientRect.left;
    i32 ClientHeight = ClientRect.bottom - ClientRect.top;

    StretchDIBits(WindowDC,
                  0, 0, ClientWidth, ClientHeight,
                  0, 0, SCREEN_W, SCREEN_H,
                  BackPixels, &ScreenBitmapInfo,
                  DIB_RGB_COLORS, SRCCOPY);
}

// --- Frame pacing ---
const i32 TARGET_FPS = 60;

typedef DWORD (WINAPI* TimeBeginPeriodFunc)(UINT);
typedef DWORD (WINAPI* TimeEndPeriodFunc)(UINT);

static LARGE_INTEGER PacingFrequency;
static HANDLE FrameTimer = 0;
static HMODULE WinmmLibrary = 0;

void PacingSetup()
{
    QueryPerformanceFrequency(&PacingFrequency);

    // winmm is loaded dynamically, so the link list stays user32 + gdi32.
    // timeBeginPeriod(1) is what makes Sleep and waitable timer waits accurate to
    // about 1 ms; without it the system timer tick is ~15.6 ms and a 60 fps cap
    // really lands near 32 fps.
    WinmmLibrary = LoadLibraryA("winmm.dll");
    if (WinmmLibrary)
    {
        TimeBeginPeriodFunc BeginPeriod = (TimeBeginPeriodFunc)GetProcAddress(WinmmLibrary, "timeBeginPeriod");
        if (BeginPeriod)
        {
            BeginPeriod(1);
        }
    }

    FrameTimer = CreateWaitableTimerW(0, FALSE, 0);
}

void PacingWait(LONGLONG UsedTicks)
{
    LONGLONG TicksPerFrame = PacingFrequency.QuadPart / LONGLONG(TARGET_FPS);
    LONGLONG IdleTicks = TicksPerFrame - UsedTicks;
    if (IdleTicks <= 0) return;

    if (FrameTimer)
    {
        // A relative waitable timer deadline, in 100 nanosecond intervals (negative).
        LARGE_INTEGER DueTime;
        DueTime.QuadPart = -(IdleTicks * 10000000) / PacingFrequency.QuadPart;

        BOOL SetOk = SetWaitableTimer(FrameTimer, &DueTime, 0, 0, 0, FALSE);
        if (SetOk)
        {
            WaitForSingleObject(FrameTimer, INFINITE);
            return;
        }
    }

    // Fallback if the waitable timer could not be created or armed.
    LONGLONG IdleMilliseconds = (IdleTicks * 1000) / PacingFrequency.QuadPart;
    if (IdleMilliseconds > 0)
    {
        Sleep((DWORD)IdleMilliseconds);
    }
}

void PacingShutdown()
{
    if (FrameTimer)
    {
        CancelWaitableTimer(FrameTimer);
        CloseHandle(FrameTimer);
        FrameTimer = 0;
    }

    if (WinmmLibrary)
    {
        TimeEndPeriodFunc EndPeriod = (TimeEndPeriodFunc)GetProcAddress(WinmmLibrary, "timeEndPeriod");
        if (EndPeriod)
        {
            EndPeriod(1);
        }
        FreeLibrary(WinmmLibrary);
        WinmmLibrary = 0;
    }
}

// Everything that happens in one frame, so the pacing loop below stays short.
void RunOneFrame()
{
    static f32 FPSTimer = 0.0f;
    static f32 WorkSecondsInWindow = 0.0f;
    static f32 FlushTimer = 0.0f;
    static i32 FramesInWindow = 0;
    static bool PrevEscapeForClose = false;

    LARGE_INTEGER WorkStart;
    QueryPerformanceCounter(&WorkStart);

    // Whether the mouse was captured at the start of this frame, before PollInput
    // can release it. ESC while captured only releases the mouse; ESC while not
    // captured closes the window.
    bool wasCaptured = MouseCaptured;

    PollInput();

    MSG Message;
    while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {
        if (Message.message == WM_QUIT)
        {
            Running = FALSE;
        }
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }
    if (!Running) return; // WM_DESTROY or WM_QUIT already ended the loop

    Dt = FrameClock();
    if (Dt > 0.1f)
    {
        Dt = 0.1f;
    }

    bool escapePressedForClose = In.escape && !PrevEscapeForClose;
    PrevEscapeForClose = In.escape;

    if (!DumpMode && !wasCaptured && escapePressedForClose)
    {
        // DestroyWindow goes through WM_DESTROY, so whatever later steps clean up or
        // save on exit runs on this path too.
        DestroyWindow(MainWindow);
        Running = FALSE;
        return;
    }

    UpdateCameraWithArrowKeys();

    FlushTimer += Dt;
    if (FlushTimer >= 0.25f)
    {
        DebugFlush();
        FlushTimer = 0.0f;
    }

    // A minimized window has no client area to draw into; skip the work but keep
    // pumping messages and pacing, so restoring the window is instant and correct.
    if (IsIconic(MainWindow)) return;

    SetupCamera();
    ClearScreen(MakeColor(120, 170, 230));
    ClearDepth();
    DrawTestTriangles3D();
    PresentBackbuffer(MainWindow);

    // Measured work, not just the frame interval: with --nocap this is the real render
    // cost, and with the 60 fps cap it stays well under 16666.
    LARGE_INTEGER WorkEnd;
    QueryPerformanceCounter(&WorkEnd);
    WorkSecondsInWindow += f32(WorkEnd.QuadPart - WorkStart.QuadPart) / f32(PacingFrequency.QuadPart);

    ++FramesInWindow;
    FPSTimer += Dt;
    if (FPSTimer >= 0.5f)
    {
        CurrentFPS = i32(f32(FramesInWindow) / FPSTimer);
        i32 USPerFrame = i32((WorkSecondsInWindow / f32(FramesInWindow)) * 1000000.0f);
        DebugSet(0, "FPS", CurrentFPS);
        DebugSet(7, "US", USPerFrame);
        FPSTimer = 0.0f;
        WorkSecondsInWindow = 0.0f;
        FramesInWindow = 0;
    }

    DumpTick(MainWindow);
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
    SetProcessDPIAware();
    DumpBegin(CommandLine);
    NoCapMode = CommandLineHas(CommandLine, "--nocap");

    WNDCLASS WindowClass;
    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc = WindowCallback;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;
    WindowClass.hInstance = Instance;
    WindowClass.hIcon = 0;
    WindowClass.hCursor = LoadCursorA(0, IDC_ARROW);
    WindowClass.hbrBackground = 0;
    WindowClass.lpszMenuName = 0;
    WindowClass.lpszClassName = "ReunionWindowClass";

    if (!RegisterClassA(&WindowClass))
    {
        return 0;
    }

    // 960x540 client area.
    RECT WindowRect;
    WindowRect.left = 0;
    WindowRect.top = 0;
    WindowRect.right = 960;
    WindowRect.bottom = 540;
    AdjustWindowRect(&WindowRect, WS_OVERLAPPEDWINDOW, FALSE);

    MainWindow = CreateWindowExA(
        0,
        WindowClass.lpszClassName,
        "Reunion Engine",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        WindowRect.right - WindowRect.left,
        WindowRect.bottom - WindowRect.top,
        0,
        0,
        Instance,
        0);

    if (!MainWindow)
    {
        return 0;
    }

    GlobalWindow = MainWindow;

    // The step 4 test wrote MOUSEX/MOUSEY into slots 1 and 2 and step 7 removes that test.
    // DebugFlush prints every slot whose label is not 0, so clear slot 2 once here.
    DebugSet(2, 0, 0);

    // CS_OWNDC gives this window one permanent device context; grab it once.
    WindowDC = GetDC(MainWindow);

    // Zeroed before filling it, so bmiColors is not uninitialized memory.
    MemZero(&ScreenBitmapInfo, (i32)sizeof(BITMAPINFO));
    ScreenBitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    ScreenBitmapInfo.bmiHeader.biWidth = SCREEN_W;
    ScreenBitmapInfo.bmiHeader.biHeight = -SCREEN_H; // top-down
    ScreenBitmapInfo.bmiHeader.biPlanes = 1;
    ScreenBitmapInfo.bmiHeader.biBitCount = 32;
    ScreenBitmapInfo.bmiHeader.biCompression = BI_RGB;

    // Frame pacing: cap the loop at TARGET_FPS instead of running flat out.
    PacingSetup();

    while (Running)
    {
        LARGE_INTEGER FrameStart;
        QueryPerformanceCounter(&FrameStart);

        RunOneFrame();

        LARGE_INTEGER FrameEnd;
        QueryPerformanceCounter(&FrameEnd);

        // --nocap is for measurements only: it runs flat out so the frame cost is visible.
        if (!NoCapMode && Running)
        {
            PacingWait(FrameEnd.QuadPart - FrameStart.QuadPart);
        }
    }

    PacingShutdown();

    return 0;
}

// ============================================================================
// Visual analysis tool - NOT part of the engine. Runs only with mc.exe --dump.
// Writes build\dump_info.txt (client size, window size, dpiAware, screen size,
// window title), build\dump_backbuffer.bmp (BackPixels, 320x180) and
// build\dump_screen.bmp (what the window DC really holds), then closes itself.
// Run it from the repository root: the paths below are relative.
// ============================================================================

static LARGE_INTEGER DumpPerfFrequency;
static LARGE_INTEGER DumpStartCounter;
static LARGE_INTEGER DumpLastCounter;

static const i32 BMP_MAX_W = 1920;
static const i32 BMP_MAX_H = 1080;
static u8 BmpPixels[BMP_MAX_W * BMP_MAX_H * 3];

void StoreU32(u8* dst, u32 value)
{
    dst[0] = u8(value & 0xFF);
    dst[1] = u8((value >> 8) & 0xFF);
    dst[2] = u8((value >> 16) & 0xFF);
    dst[3] = u8((value >> 24) & 0xFF);
}

void StoreU16(u8* dst, u16 value)
{
    dst[0] = u8(value & 0xFF);
    dst[1] = u8((value >> 8) & 0xFF);
}

// Generic 24bpp BMP writer from a top-down 32bpp pixel array (0x00RRGGBB).
// Header field offsets follow BITMAPINFOHEADER: biSize 0, biWidth 4, biHeight 8,
// biPlanes 12, biBitCount 14, biCompression 16, biSizeImage 20, biXppm 24,
// biYppm 28, biClrUsed 32, biClrImportant 36.
void SaveBmpFromU32(const u32* pixels, i32 pitchPixels, i32 width, i32 height, const char* path)
{
    if (width <= 0 || height <= 0) return;
    if (width > BMP_MAX_W || height > BMP_MAX_H) return;

    // BMP rows run bottom-up and every row is padded to a 4 byte boundary.
    i32 RowBytes = width * 3;
    i32 Padding = (4 - (RowBytes % 4)) % 4;
    i32 RowPitch = RowBytes + Padding;
    u32 PixelBytes = (u32)(RowPitch * height);
    u32 FileSize = 14 + 40 + PixelBytes;

    u8 header[14];
    header[0] = 'B';
    header[1] = 'M';
    StoreU32(&header[2], FileSize);
    StoreU16(&header[6], 0);   // reserved1
    StoreU16(&header[8], 0);   // reserved2
    StoreU32(&header[10], 54); // pixel data offset

    u8 info[40];
    for (i32 i = 0; i < 40; ++i) info[i] = 0;
    StoreU32(&info[0], 40);          // biSize
    StoreU32(&info[4], (u32)width);  // biWidth
    StoreU32(&info[8], (u32)height); // biHeight, positive = bottom-up rows
    StoreU16(&info[12], 1);          // biPlanes
    StoreU16(&info[14], 24);         // biBitCount
    StoreU32(&info[16], 0);          // biCompression = BI_RGB
    StoreU32(&info[20], PixelBytes); // biSizeImage
    StoreU32(&info[24], 0);          // biXPelsPerMeter
    StoreU32(&info[28], 0);          // biYPelsPerMeter
    StoreU32(&info[32], 0);          // biClrUsed
    StoreU32(&info[36], 0);          // biClrImportant

    i32 out = 0;
    for (i32 y = height - 1; y >= 0; --y) // BMP rows go bottom-up
    {
        for (i32 x = 0; x < width; ++x)
        {
            u32 color = pixels[y * pitchPixels + x];
            BmpPixels[out] = u8(color & 0xFF);
            BmpPixels[out + 1] = u8((color >> 8) & 0xFF);
            BmpPixels[out + 2] = u8((color >> 16) & 0xFF);
            out += 3;
        }

        for (i32 p = 0; p < Padding; ++p)
        {
            BmpPixels[out] = 0;
            ++out;
        }
    }

    DWORD Written = 0;
    HANDLE File = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (File != INVALID_HANDLE_VALUE)
    {
        WriteFile(File, header, 14, &Written, 0);
        WriteFile(File, info, 40, &Written, 0);
        WriteFile(File, BmpPixels, PixelBytes, &Written, 0);
        CloseHandle(File);
    }
}

void SaveDumpBmp(const char* path)
{
    SaveBmpFromU32(BackPixels, SCREEN_W, SCREEN_W, SCREEN_H, path);
}

// Screen capture: read back what the window's device context really holds.
static HDC CaptureMemDC = 0;
static HBITMAP CaptureBitmap = 0;
static HBITMAP CaptureOldBitmap = 0;
static void* CaptureBits = 0;
static BITMAPINFO CaptureInfo;
static i32 CaptureWidth = 0;
static i32 CaptureHeight = 0;

void AppendNumber(char* buffer, i32 value)
{
    if (value < 0)
    {
        lstrcatA(buffer, "-");
        value = -value;
    }

    char digits[16];
    i32 digitCount = 0;
    while (digitCount < 16)
    {
        digits[digitCount] = u8('0' + (value % 10));
        value /= 10;
        if (value == 0) break;
        ++digitCount;
    }

    for (i32 i = digitCount; i >= 0; --i)
    {
        char one[2];
        one[0] = digits[i];
        one[1] = 0;
        lstrcatA(buffer, one);
    }
}

void DumpInfo(HWND Window, const char* path)
{
    RECT ClientRect;
    RECT WindowRect;
    GetClientRect(Window, &ClientRect);
    GetWindowRect(Window, &WindowRect);

    char Line[256];
    Line[0] = 0;
    lstrcatA(Line, "client ");
    AppendNumber(Line, ClientRect.right - ClientRect.left);
    lstrcatA(Line, "x");
    AppendNumber(Line, ClientRect.bottom - ClientRect.top);
    lstrcatA(Line, " | window ");
    AppendNumber(Line, WindowRect.right - WindowRect.left);
    lstrcatA(Line, "x");
    AppendNumber(Line, WindowRect.bottom - WindowRect.top);
    lstrcatA(Line, " | dpiAware ");
    AppendNumber(Line, (i32)IsProcessDPIAware());
    lstrcatA(Line, " | screen ");
    AppendNumber(Line, GetSystemMetrics(SM_CXSCREEN));
    lstrcatA(Line, "x");
    AppendNumber(Line, GetSystemMetrics(SM_CYSCREEN));
    lstrcatA(Line, "\r\n");

    char Title[128];
    GetWindowTextA(Window, Title, 128);
    Title[127] = 0;
    lstrcatA(Line, "title: ");
    lstrcatA(Line, Title);
    lstrcatA(Line, "\r\n");

    DWORD Written = 0;
    HANDLE File = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (File != INVALID_HANDLE_VALUE)
    {
        WriteFile(File, Line, lstrlenA(Line), &Written, 0);
        CloseHandle(File);
    }
}

BOOL CaptureWindowPixels(HWND Window, const char* path)
{
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    i32 Width = ClientRect.right - ClientRect.left;
    i32 Height = ClientRect.bottom - ClientRect.top;
    if (Width > BMP_MAX_W) Width = BMP_MAX_W;
    if (Height > BMP_MAX_H) Height = BMP_MAX_H;

    if (Width != CaptureWidth || Height != CaptureHeight)
    {
        // Delete in the right order: our bitmap is selected out of the DC first.
        if (CaptureBitmap)
        {
            if (CaptureMemDC && CaptureOldBitmap)
            {
                SelectObject(CaptureMemDC, CaptureOldBitmap);
            }
            DeleteObject(CaptureBitmap);
            CaptureBitmap = 0;
            CaptureOldBitmap = 0;
            CaptureBits = 0;
        }

        if (CaptureMemDC)
        {
            DeleteDC(CaptureMemDC);
            CaptureMemDC = 0;
        }

        HDC ReferenceDC = GetDC(Window);
        CaptureMemDC = CreateCompatibleDC(ReferenceDC);
        ReleaseDC(Window, ReferenceDC);
        if (!CaptureMemDC) return FALSE;

        MemZero(&CaptureInfo, (i32)sizeof(BITMAPINFO));
        CaptureInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        CaptureInfo.bmiHeader.biWidth = Width;
        CaptureInfo.bmiHeader.biHeight = -Height; // top-down
        CaptureInfo.bmiHeader.biPlanes = 1;
        CaptureInfo.bmiHeader.biBitCount = 32;
        CaptureInfo.bmiHeader.biCompression = BI_RGB;

        CaptureBitmap = CreateDIBSection(CaptureMemDC, &CaptureInfo, DIB_RGB_COLORS, &CaptureBits, 0, 0);
        if (!CaptureBitmap)
        {
            DeleteDC(CaptureMemDC);
            CaptureMemDC = 0;
            return FALSE;
        }

        CaptureOldBitmap = (HBITMAP)SelectObject(CaptureMemDC, CaptureBitmap);
        CaptureWidth = Width;
        CaptureHeight = Height;
    }

    HDC DeviceContext = GetDC(Window);
    BOOL Blitted = BitBlt(CaptureMemDC, 0, 0, Width, Height, DeviceContext, 0, 0, SRCCOPY);
    ReleaseDC(Window, DeviceContext);

    if (!Blitted) return FALSE;

    SaveBmpFromU32((const u32*)CaptureBits, Width, Width, Height, path);
    return TRUE;
}

void DumpBegin(LPSTR commandLine)
{
    DumpMode = FALSE;
    if (!CommandLineHas(commandLine, "--dump")) return;

    DumpMode = TRUE;
    QueryPerformanceFrequency(&DumpPerfFrequency);
    QueryPerformanceCounter(&DumpStartCounter);
    DumpLastCounter = DumpStartCounter;
}

void DumpTick(HWND Window)
{
    if (!DumpMode || !Running) return;
    if (!IsWindow(Window)) return;

    LARGE_INTEGER Now;
    QueryPerformanceCounter(&Now);
    f32 SecondsNow = f32(Now.QuadPart - DumpStartCounter.QuadPart) / f32(DumpPerfFrequency.QuadPart);
    f32 SecondsSinceDump = f32(Now.QuadPart - DumpLastCounter.QuadPart) / f32(DumpPerfFrequency.QuadPart);

    if (SecondsSinceDump >= 0.5f)
    {
        DumpLastCounter = Now;
        DumpInfo(Window, "build\\dump_info.txt");
        SaveDumpBmp("build\\dump_backbuffer.bmp");
        CaptureWindowPixels(Window, "build\\dump_screen.bmp");
    }

    if (SecondsNow >= 1.5f)
    {
        PostMessageA(Window, WM_CLOSE, 0, 0);
    }
}
