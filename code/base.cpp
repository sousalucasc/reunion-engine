// base.cpp - types, screen buffer, pixel helpers. Included first by main.cpp.

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef int            i32;
typedef float          f32;

const i32 SCREEN_W = 320;
const i32 SCREEN_H = 180;

static u32 BackPixels[SCREEN_W * SCREEN_H];

u32 MakeColor(u8 r, u8 g, u8 b)
{
    return (u32(r) << 16) | (u32(g) << 8) | u32(b);
}

void PutPixel(i32 x, i32 y, u32 color)
{
    if (x < 0 || x >= SCREEN_W) return;
    if (y < 0 || y >= SCREEN_H) return;
    BackPixels[y * SCREEN_W + x] = color;
}

void ClearScreen(u32 color)
{
    for (i32 i = 0; i < SCREEN_W * SCREEN_H; ++i)
    {
        BackPixels[i] = color;
    }
}

// --- Timing and debug output (step 2) ---
HWND GlobalWindow = 0;
f32 Dt = 0.016f;
i32 CurrentFPS = 0;

struct DebugSlot { const char* label; i32 value; };
static DebugSlot DebugSlots[8];

void DebugSet(i32 slot, const char* label, i32 value)
{
    if (slot < 0 || slot > 7) return;
    DebugSlots[slot].label = label;
    DebugSlots[slot].value = value;
}

void DebugFlush()
{
    char buffer[512];
    buffer[0] = 0;

    for (i32 i = 0; i < 8; ++i)
    {
        if (DebugSlots[i].label == 0) continue;

        if (buffer[0] != 0)
        {
            lstrcatA(buffer, " | ");
        }
        lstrcatA(buffer, DebugSlots[i].label);
        lstrcatA(buffer, ": ");

        char number[32];
        wsprintfA(number, "%d", DebugSlots[i].value);
        lstrcatA(buffer, number);
    }

    SetWindowTextA(GlobalWindow, buffer);
}

static LARGE_INTEGER PerfFrequency;
static LARGE_INTEGER LastCounter;
static BOOL ClockStarted = FALSE;

f32 FrameClock()
{
    if (!ClockStarted)
    {
        QueryPerformanceFrequency(&PerfFrequency);
        QueryPerformanceCounter(&LastCounter);
        ClockStarted = TRUE;
        return 0.016f;
    }

    LARGE_INTEGER Now;
    QueryPerformanceCounter(&Now);

    f32 seconds = f32(Now.QuadPart - LastCounter.QuadPart) / f32(PerfFrequency.QuadPart);
    LastCounter = Now;

    return seconds;
}

// --- Math library (step 3) ---
const f32 PI = 3.14159265f;
const f32 TAU = 6.28318531f;
const f32 NEAR_W = 0.05f;

f32 Abs(f32 x)
{
    return (x < 0.0f) ? -x : x;
}

i32 AbsI(i32 x)
{
    return (x < 0) ? -x : x;
}

f32 Min(f32 a, f32 b)
{
    return (a < b) ? a : b;
}

f32 Max(f32 a, f32 b)
{
    return (a > b) ? a : b;
}

i32 MinI(i32 a, i32 b)
{
    return (a < b) ? a : b;
}

i32 MaxI(i32 a, i32 b)
{
    return (a > b) ? a : b;
}

f32 Clamp(f32 v, f32 lo, f32 hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

f32 Lerp(f32 a, f32 b, f32 t)
{
    return a + (b - a) * t;
}

f32 Floor(f32 x)
{
    i32 i = (i32)x;
    return (x >= (f32)i) ? (f32)i : (f32)(i - 1);
}

i32 FloorI(f32 x)
{
    i32 i = (i32)x;
    return (x >= (f32)i) ? i : (i - 1);
}

i32 FloorDiv(i32 a, i32 b)
{
    // b > 0. a / b rounded toward negative infinity.
    if (a >= 0) return a / b;
    return -((-a + b - 1) / b);
}

i32 Mod(i32 a, i32 b)
{
    // b > 0. Result always in 0..b-1.
    return a - FloorDiv(a, b) * b;
}

f32 Sqrt(f32 x)
{
    if (x <= 0.0f) return 0.0f;

    f32 guess = (x > 1.0f) ? x * 0.5f : 1.0f;
    for (i32 i = 0; i < 24; ++i)
    {
        guess = 0.5f * (guess + x / guess);
    }

    return guess;
}

f32 Sin(f32 x)
{
    // Wrap into -PI..PI.
    x = x - TAU * Floor(x / TAU + 0.5f);

    // Fold into -PI/2..PI/2 so the Taylor series stays accurate.
    if (x > PI * 0.5f) x = PI - x;
    if (x < -PI * 0.5f) x = -PI - x;

    f32 x2 = x * x;
    f32 x3 = x2 * x;
    f32 x5 = x3 * x2;
    f32 x7 = x5 * x2;
    f32 x9 = x7 * x2;

    return x - x3 / 6.0f + x5 / 120.0f - x7 / 5040.0f + x9 / 362880.0f;
}

f32 Cos(f32 x)
{
    return Sin(x + PI * 0.5f);
}

void MemZero(void* p, i32 bytes)
{
    u8* destination = (u8*)p;
    for (i32 i = 0; i < bytes; ++i)
    {
        destination[i] = 0;
    }
}

void MemCopy(void* dst, const void* src, i32 bytes)
{
    u8* destination = (u8*)dst;
    const u8* source = (const u8*)src;
    for (i32 i = 0; i < bytes; ++i)
    {
        destination[i] = source[i];
    }
}

struct v3 { f32 x, y, z; };
struct v4 { f32 x, y, z, w; };
struct m4 { f32 m[4][4]; };

v3 V3(f32 x, f32 y, f32 z)
{
    v3 result;
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

v3 V3Add(v3 a, v3 b)
{
    return V3(a.x + b.x, a.y + b.y, a.z + b.z);
}

v3 V3Sub(v3 a, v3 b)
{
    return V3(a.x - b.x, a.y - b.y, a.z - b.z);
}

v3 V3Scale(v3 a, f32 s)
{
    return V3(a.x * s, a.y * s, a.z * s);
}

f32 V3Dot(v3 a, v3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

v3 V3Cross(v3 a, v3 b)
{
    return V3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

f32 V3Len(v3 a)
{
    return Sqrt(V3Dot(a, a));
}

v3 V3Norm(v3 a)
{
    f32 len = V3Len(a);
    if (len < 0.000001f) return a;
    return V3Scale(a, 1.0f / len);
}

v4 V4(f32 x, f32 y, f32 z, f32 w)
{
    v4 result;
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    return result;
}

m4 M4Identity()
{
    m4 result;
    MemZero(&result, (i32)sizeof(m4));
    result.m[0][0] = 1.0f;
    result.m[1][1] = 1.0f;
    result.m[2][2] = 1.0f;
    result.m[3][3] = 1.0f;
    return result;
}

m4 M4Mul(m4 a, m4 b)
{
    // result = a * b
    m4 result;
    MemZero(&result, (i32)sizeof(m4));

    for (i32 row = 0; row < 4; ++row)
    {
        for (i32 col = 0; col < 4; ++col)
        {
            f32 sum = 0.0f;
            for (i32 k = 0; k < 4; ++k)
            {
                sum += a.m[row][k] * b.m[k][col];
            }
            result.m[row][col] = sum;
        }
    }

    return result;
}

v4 M4MulV4(m4 m, v4 v)
{
    v4 result;
    result.x = m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z + m.m[0][3] * v.w;
    result.y = m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z + m.m[1][3] * v.w;
    result.z = m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z + m.m[2][3] * v.w;
    result.w = m.m[3][0] * v.x + m.m[3][1] * v.y + m.m[3][2] * v.z + m.m[3][3] * v.w;
    return result;
}

m4 M4Perspective(f32 fovY, f32 aspect)
{
    m4 result;
    MemZero(&result, (i32)sizeof(m4));

    f32 f = Cos(fovY * 0.5f) / Sin(fovY * 0.5f);
    result.m[0][0] = f / aspect;
    result.m[1][1] = f;
    result.m[2][2] = 1.0f;
    result.m[3][2] = 1.0f; // clip.w = view z

    return result;
}

m4 M4View(v3 pos, f32 yaw, f32 pitch)
{
    v3 forward = V3(Sin(yaw) * Cos(pitch), Sin(pitch), Cos(yaw) * Cos(pitch));
    v3 right = V3(Cos(yaw), 0.0f, -Sin(yaw));
    v3 up = V3Cross(forward, right);

    m4 result;
    MemZero(&result, (i32)sizeof(m4));

    result.m[0][0] = right.x;
    result.m[0][1] = right.y;
    result.m[0][2] = right.z;
    result.m[0][3] = -V3Dot(right, pos);

    result.m[1][0] = up.x;
    result.m[1][1] = up.y;
    result.m[1][2] = up.z;
    result.m[1][3] = -V3Dot(up, pos);

    result.m[2][0] = forward.x;
    result.m[2][1] = forward.y;
    result.m[2][2] = forward.z;
    result.m[2][3] = -V3Dot(forward, pos);

    result.m[3][0] = 0.0f;
    result.m[3][1] = 0.0f;
    result.m[3][2] = 0.0f;
    result.m[3][3] = 1.0f;

    return result;
}

// --- Input (step 4) ---
struct Input
{
    bool w, a, s, d, up, down, left, right, space, ctrl, shift, escape, lmb, rmb;
    bool lmbPressed, rmbPressed;
    bool f3, f3Pressed, f5Pressed, t, f, fPressed;
    bool keyNum[5];
    i32 mouseDX, mouseDY;
};

Input In;
bool MouseCaptured = false;

static bool PrevLmb = false;
static bool PrevRmb = false;
static bool PrevF3 = false;
static bool PrevF5 = false;
static bool PrevF = false;
static bool PrevEscape = false;

void PollInput()
{
    // Only the focused window reads input; everything else is treated as "nothing pressed".
    if (GetForegroundWindow() != GlobalWindow)
    {
        In.w = false;
        In.a = false;
        In.s = false;
        In.d = false;
        In.up = false;
        In.down = false;
        In.left = false;
        In.right = false;
        In.space = false;
        In.ctrl = false;
        In.shift = false;
        In.escape = false;
        In.lmb = false;
        In.rmb = false;
        In.lmbPressed = false;
        In.rmbPressed = false;
        In.f3 = false;
        In.f3Pressed = false;
        In.f5Pressed = false;
        In.t = false;
        In.f = false;
        In.fPressed = false;
        In.keyNum[0] = false;
        In.keyNum[1] = false;
        In.keyNum[2] = false;
        In.keyNum[3] = false;
        In.keyNum[4] = false;
        In.mouseDX = 0;
        In.mouseDY = 0;
        return;
    }

    bool wDown = (GetAsyncKeyState('W') & 0x8000) != 0;
    bool aDown = (GetAsyncKeyState('A') & 0x8000) != 0;
    bool sDown = (GetAsyncKeyState('S') & 0x8000) != 0;
    bool dDown = (GetAsyncKeyState('D') & 0x8000) != 0;
    bool upDown = (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
    bool downDown = (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;
    bool leftDown = (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
    bool rightDown = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
    bool spaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
    bool ctrlDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool shiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    bool escapeDown = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
    bool lmbDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    bool rmbDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    bool f3Down = (GetAsyncKeyState(VK_F3) & 0x8000) != 0;
    bool f5Down = (GetAsyncKeyState(VK_F5) & 0x8000) != 0;
    bool tDown = (GetAsyncKeyState('T') & 0x8000) != 0;
    bool fDown = (GetAsyncKeyState('F') & 0x8000) != 0;

    In.w = wDown;
    In.a = aDown;
    In.s = sDown;
    In.d = dDown;
    In.up = upDown;
    In.down = downDown;
    In.left = leftDown;
    In.right = rightDown;
    In.space = spaceDown;
    In.ctrl = ctrlDown;
    In.shift = shiftDown;
    In.escape = escapeDown;
    In.lmb = lmbDown;
    In.rmb = rmbDown;
    In.f3 = f3Down;
    In.t = tDown;
    In.f = fDown;

    In.lmbPressed = lmbDown && !PrevLmb;
    In.rmbPressed = rmbDown && !PrevRmb;
    In.f3Pressed = f3Down && !PrevF3;
    In.f5Pressed = f5Down && !PrevF5;
    In.fPressed = fDown && !PrevF;

    PrevLmb = lmbDown;
    PrevRmb = rmbDown;
    PrevF3 = f3Down;
    PrevF5 = f5Down;
    PrevF = fDown;

    In.keyNum[0] = (GetAsyncKeyState('1') & 0x8000) != 0;
    In.keyNum[1] = (GetAsyncKeyState('2') & 0x8000) != 0;
    In.keyNum[2] = (GetAsyncKeyState('3') & 0x8000) != 0;
    In.keyNum[3] = (GetAsyncKeyState('4') & 0x8000) != 0;
    In.keyNum[4] = (GetAsyncKeyState('5') & 0x8000) != 0;

    bool escapePressed = escapeDown && !PrevEscape;
    PrevEscape = escapeDown;

    if (MouseCaptured)
    {
        // Centre of the client area, in screen coordinates.
        RECT ClientRect;
        GetClientRect(GlobalWindow, &ClientRect);
        POINT Center;
        Center.x = (ClientRect.right - ClientRect.left) / 2;
        Center.y = (ClientRect.bottom - ClientRect.top) / 2;
        ClientToScreen(GlobalWindow, &Center);

        POINT Cursor;
        GetCursorPos(&Cursor);

        In.mouseDX = Cursor.x - Center.x;
        In.mouseDY = Cursor.y - Center.y;

        SetCursorPos(Center.x, Center.y);
    }
    else
    {
        In.mouseDX = 0;
        In.mouseDY = 0;
    }

    if (!MouseCaptured && In.lmbPressed)
    {
        MouseCaptured = true;
        ShowCursor(FALSE);
    }

    if (MouseCaptured && escapePressed)
    {
        MouseCaptured = false;
        ShowCursor(TRUE);
    }
}
