// main.cpp - the single translation unit. Own file includes only, in order.
#include <windows.h>
#include "base.cpp"
#include "raster.cpp"

static BOOL Running = TRUE;
static HWND MainWindow = 0;
static HDC WindowDC = 0;
static BITMAPINFO ScreenBitmapInfo;
static i32 FrameCounter = 0;
static BOOL DumpMode = FALSE;

// The visual-analysis tool lives at the bottom of this file, after the engine code.
// Declared here so WinMain can start it and tick it. Without --dump it never runs.
void DumpBegin(LPSTR commandLine);
void DumpTick(HWND Window);

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
            // CS_OWNDC: this is the window's one device context, released once here.
            if (WindowDC)
            {
                ReleaseDC(Window, WindowDC);
                WindowDC = 0;
            }
            Running = FALSE;
            PostQuitMessage(0);
        } break;

        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }

    return Result;
}

void FillGradient(u32 blue)
{
    // Continuous red across the width: red = x would wrap at x = 256 (u8).
    for (i32 y = 0; y < SCREEN_H; ++y)
    {
        for (i32 x = 0; x < SCREEN_W; ++x)
        {
            u8 red = u8(f32(x) * 255.0f / f32(SCREEN_W - 1));
            PutPixel(x, y, MakeColor(red, u8(y), u8(blue)));
        }
    }
}

// --- Temporary test for step 4 input ---
static f32 SquareX = 150.0f;
static f32 SquareY = 85.0f;

void UpdateAndDrawTestSquare()
{
    const f32 SQUARE_SPEED = 100.0f; // pixels per second
    const i32 SQUARE_SIZE = 10;

    if (In.w) SquareY -= SQUARE_SPEED * Dt;
    if (In.s) SquareY += SQUARE_SPEED * Dt;
    if (In.a) SquareX -= SQUARE_SPEED * Dt;
    if (In.d) SquareX += SQUARE_SPEED * Dt;

    DebugSet(1, "MOUSEX", In.mouseDX);
    DebugSet(2, "MOUSEY", In.mouseDY);

    i32 left = FloorI(SquareX);
    i32 top = FloorI(SquareY);

    for (i32 y = 0; y < SQUARE_SIZE; ++y)
    {
        for (i32 x = 0; x < SQUARE_SIZE; ++x)
        {
            PutPixel(left + x, top + y, MakeColor(255, 255, 255));
        }
    }
}

// --- Temporary test for step 6: depth buffer ---
static ScreenVert RotateScreenVert(ScreenVert v, f32 centerX, f32 centerY, f32 angle)
{
    ScreenVert result;
    result.x = centerX + Cos(angle) * (v.x - centerX) - Sin(angle) * (v.y - centerY);
    result.y = centerY + Sin(angle) * (v.x - centerX) + Cos(angle) * (v.y - centerY);
    result.invW = v.invW;
    return result;
}

void DrawTestTriangles()
{
    // Triangle A: a wide triangle, all three vertices invW = 0.2.
    ScreenVert A1 = { 100.0f, 40.0f, 0.2f };
    ScreenVert A2 = { 220.0f, 40.0f, 0.2f };
    ScreenVert A3 = { 160.0f, 120.0f, 0.2f };

    // Triangle B: the same shape rotated about its centre so it overlaps A, slanting in
    // depth: one vertex invW = 0.1 (farther than A), the other two invW = 0.4 (nearer).
    f32 centreX = (100.0f + 220.0f + 160.0f) / 3.0f;
    f32 centreY = (40.0f + 40.0f + 120.0f) / 3.0f;
    f32 angle = 0.35f;

    ScreenVert B1 = RotateScreenVert(A1, centreX, centreY, angle);
    ScreenVert B2 = RotateScreenVert(A2, centreX, centreY, angle);
    ScreenVert B3 = RotateScreenVert(A3, centreX, centreY, angle);
    B1.invW = 0.1f;
    B2.invW = 0.4f;
    B3.invW = 0.4f;

    // Normal order: B first, then A. Holding SPACE swaps the order, so the depth test
    // decides who wins instead of the draw order.
    if (In.space)
    {
        DS.FlatColor = MakeColor(255, 0, 0);
        RasterTriangle(A1, A2, A3);
        DS.FlatColor = MakeColor(0, 0, 255);
        RasterTriangle(B1, B2, B3);
    }
    else
    {
        DS.FlatColor = MakeColor(0, 0, 255);
        RasterTriangle(B1, B2, B3);
        DS.FlatColor = MakeColor(255, 0, 0);
        RasterTriangle(A1, A2, A3);
    }
}

void PresentBackbuffer(HWND Window)
{
    // The CS_OWNDC device context is held from window creation until WM_DESTROY,
    // so no HDC is created or freed per frame.
    if (WindowDC == 0) return;

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
    static f32 FlushTimer = 0.0f;
    static i32 FramesInWindow = 0;
    static bool PrevEscapeForClose = false;

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

    Dt = FrameClock();
    if (Dt > 0.1f)
    {
        Dt = 0.1f;
    }

    ++FramesInWindow;
    FPSTimer += Dt;
    if (FPSTimer >= 0.5f)
    {
        CurrentFPS = i32(f32(FramesInWindow) / FPSTimer);
        DebugSet(0, "FPS", CurrentFPS);
        FPSTimer = 0.0f;
        FramesInWindow = 0;
    }

    FlushTimer += Dt;
    if (FlushTimer >= 0.25f)
    {
        DebugFlush();
        FlushTimer = 0.0f;
    }

    bool escapePressedForClose = In.escape && !PrevEscapeForClose;
    PrevEscapeForClose = In.escape;

    if (!DumpMode && !wasCaptured && escapePressedForClose)
    {
        PostMessageA(MainWindow, WM_CLOSE, 0, 0);
        Running = FALSE;
    }

    ++FrameCounter;
    FillGradient(u32(FrameCounter));
    ClearDepth();
    DrawTestTriangles();
    UpdateAndDrawTestSquare();
    PresentBackbuffer(MainWindow);

    DumpTick(MainWindow);
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
    SetProcessDPIAware();
    DumpBegin(CommandLine);

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
        PacingWait(FrameEnd.QuadPart - FrameStart.QuadPart);
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
    u32 PixelBytes = (u32)(width * height * 3);
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
        if (CaptureBitmap)
        {
            SelectObject(CaptureMemDC, CaptureBitmap);
            DeleteObject(CaptureBitmap);
            DeleteDC(CaptureMemDC);
            CaptureBitmap = 0;
            CaptureMemDC = 0;
            CaptureBits = 0;
        }

        HDC ReferenceDC = GetDC(Window);
        CaptureMemDC = CreateCompatibleDC(ReferenceDC);
        ReleaseDC(Window, ReferenceDC);

        MemZero(&CaptureInfo, (i32)sizeof(BITMAPINFO));
        CaptureInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        CaptureInfo.bmiHeader.biWidth = Width;
        CaptureInfo.bmiHeader.biHeight = -Height; // top-down
        CaptureInfo.bmiHeader.biPlanes = 1;
        CaptureInfo.bmiHeader.biBitCount = 32;
        CaptureInfo.bmiHeader.biCompression = BI_RGB;

        CaptureBitmap = CreateDIBSection(CaptureMemDC, &CaptureInfo, DIB_RGB_COLORS, &CaptureBits, 0, 0);
        if (!CaptureBitmap) return FALSE;

        SelectObject(CaptureMemDC, CaptureBitmap);
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
    if (commandLine == 0) return;

    for (i32 i = 0; commandLine[i] != 0; ++i)
    {
        if (commandLine[i] == '-' && commandLine[i + 1] == '-' &&
            commandLine[i + 2] == 'd' && commandLine[i + 3] == 'u' &&
            commandLine[i + 4] == 'm' && commandLine[i + 5] == 'p')
        {
            DumpMode = TRUE;
            QueryPerformanceFrequency(&DumpPerfFrequency);
            QueryPerformanceCounter(&DumpStartCounter);
            DumpLastCounter = DumpStartCounter;
            return;
        }
    }
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
