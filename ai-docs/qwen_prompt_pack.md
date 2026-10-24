# Minecraft clone, software rendered: Qwen prompt pack

C++ with windows.h only. 36 steps. Every step ends with something you can see on screen, and you do the check (Qwen cannot see the screen).

## How to use

1. New chat for every step.
2. Paste in this order: RULES, CONVENTIONS, API.txt, the files the step lists, then the step prompt.
3. Run build.bat and do the Check. If it passes: `git commit`.
4. After a passing step, run the API prompt (below) and save the output as API.txt, replacing the old one. This keeps the context small.
5. Failed 3 times: `git checkout .` and redo the step. Still failing: use the FIX prompt with only the broken function.

build.bat (run from "x64 Native Tools Command Prompt for VS 2022"):

```
@echo off
cl /nologo /Zi /Od /W3 /Fe:mc.exe main.cpp user32.lib gdi32.lib
```

When you start caring about FPS, change `/Od` to `/O2 /fp:fast`.

---

## RULES (paste first, every time)

```
RULES (follow strictly):
- Language: C++ for Windows, MSVC x64. The ONLY allowed #include is <windows.h>.
- No STL, no CRT functions (printf, malloc, memset, memcpy, sin, cos, sqrt, floor, abs, rand). Use our own functions from base.cpp (MemZero, MemCopy, Sqrt, Sin, Cos, Floor, Abs, Min, Max...). Win32 API functions are allowed.
- Plain structs and functions only. No classes, templates, namespaces, operator overloading, new/delete, exceptions. Global/static arrays only, no dynamic allocation.
- Single translation unit: main.cpp #includes our own files in this order, skipping files that do not exist yet: base.cpp, raster.cpp, world.cpp, render.cpp, player.cpp, hud.cpp. Never add any other #include and never create other files. A file may only use things defined in files included before it, or earlier in the same file.
- Do ONLY the current step. Do not refactor, rename, reformat or "improve" existing code. Do not add features that are not listed in the step.
- Only edit the files the step names. For every change output the COMPLETE new version of each changed or new function/struct (never "...rest unchanged") and say exactly where it goes ("replace function X", "add after function Y").
- Types: u8, u16, u32, i32, f32 (defined in base.cpp). Write float literals like 1.0f.
- Prefer simple, long, explicit code over clever code. Full variable names (single letters only for loop counters and x, y, z).
- Debug output goes to the window title: DebugSet(slot, "label", value). Integers only, so multiply floats by 1000.
- End your answer with the word DONE, then exactly one line: what I should see on screen.
```

## CONVENTIONS (paste second, every time)

```
CONVENTIONS:
- World: Y up, X right, Z forward (left-handed). Block (x,y,z) occupies [x,x+1) x [y,y+1) x [z,z+1).
- Camera: yaw 0 looks along +Z, positive yaw turns toward +X.
  forward = (Sin(yaw)*Cos(pitch), Sin(pitch), Cos(yaw)*Cos(pitch)); right = (Cos(yaw), 0, -Sin(yaw)); up = Cross(forward, right). Pitch limited to +-1.5.
- Matrices: struct m4 { f32 m[4][4]; } row-major, column vectors: out = M * v (out.x = m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3]*v.w, and so on).
- View space: +X right, +Y up, +Z forward (things in front have positive z). Projection output has clip.w = view z. A point is in front of the camera when w >= NEAR_W (0.05).
- Screen: SCREEN_W x SCREEN_H = 320x180, origin top-left, y down. Pixel = u32 0x00RRGGBB.
  ndc to screen: sx = (ndcX*0.5+0.5)*SCREEN_W, sy = (0.5-ndcY*0.5)*SCREEN_H.
- Depth buffer stores invW = 1/w (bigger = nearer), cleared to 0.0f. A pixel is drawn only if invW > Depth[i].
- The triangle rasterizer accepts any winding. Back faces are culled by normal (dot(faceNormal, cameraPos - faceCenter) <= 0), never by winding.
- Chunk = 16 x 64 x 16 blocks. World to chunk: cx = FloorDiv(wx,16), local x = wx - cx*16. Sea level = 28.
- Block ids: AIR 0, GRASS 1, DIRT 2, WOOD 3, WATER 4, LEAVES 5.
- Face ids: 0 = +X, 1 = -X, 2 = +Y (top), 3 = -Y (bottom), 4 = +Z, 5 = -Z.
```

## API prompt (run after every passing step)

```
List every struct, enum, #define, const, global variable and function (signature only, no bodies) in the code below, grouped by file, one per line, no comments, no explanation. Output only the list.

[PASTE ALL .cpp FILES]
```

## FIX prompt

```
[RULES]
[CONVENTIONS]
[API.txt]
The code below was written for STEP N but it fails.
Problem: <paste the exact compiler error, OR describe what you see on screen versus the Check>
Fix ONLY this problem. Change as little as possible. Output the complete corrected function(s) and where they go. Do not touch anything else.

[PASTE ONLY THE FILE(S) INVOLVED]
```

---

# A. Base

## Step 1: Baseline (files: main.cpp, base.cpp)

```
STEP 1: Create the project skeleton from my existing code. Create two files: base.cpp and main.cpp. My Day 4 code (window + animated backbuffer gradient) is at the bottom; reuse its ideas.

base.cpp contains only:
- typedefs: u8 (unsigned char), u16 (unsigned short), u32 (unsigned int), i32 (int), f32 (float)
- const i32 SCREEN_W = 320; const i32 SCREEN_H = 180;
- static u32 BackPixels[SCREEN_W * SCREEN_H];
- u32 MakeColor(u8 r, u8 g, u8 b): returns (r << 16) | (g << 8) | b
- void PutPixel(i32 x, i32 y, u32 color): ignores out-of-range coordinates
- void ClearScreen(u32 color)

main.cpp contains:
- #include <windows.h> then #include "base.cpp"
- WinMain: register a window class (CS_HREDRAW | CS_VREDRAW | CS_OWNDC), create a window with a 960x540 CLIENT area (use AdjustWindowRect), message loop with PeekMessageA, WM_CLOSE/WM_DESTROY quit via PostQuitMessage(0).
- A BITMAPINFO for SCREEN_W x SCREEN_H, 32 bits, top-down (negative biHeight), BI_RGB.
- Every frame: fill BackPixels with an animated gradient using PutPixel (red = x, green = y, blue = frame counter), then GetDC, StretchDIBits (SRCCOPY) scaling BackPixels to the whole client rect (GetClientRect), ReleaseDC.
- ESC closes the window.

[PASTE YOUR DAY 4 CODE HERE]
```

Check: a 320x180 animated gradient scaled to fill the window. ESC and the X button close it cleanly.

## Step 2: Loop timing and title debug (files: main.cpp, base.cpp)

```
STEP 2: Frame timing and title debug output.
In base.cpp add:
- HWND GlobalWindow = 0; f32 Dt = 0.016f; i32 CurrentFPS = 0;
- struct DebugSlot { const char* label; i32 value; }; static DebugSlot DebugSlots[8];
- void DebugSet(i32 slot, const char* label, i32 value): stores label and value in DebugSlots[slot] (ignore slot outside 0..7).
- void DebugFlush(): builds ONE string like "FPS: 60 | X: 123" from every slot whose label is not 0, using wsprintfA and lstrcatA into a local char buffer[512], then SetWindowTextA(GlobalWindow, buffer).
- f32 FrameClock(): uses QueryPerformanceFrequency/QueryPerformanceCounter, returns the seconds since the previous call (first call returns 0.016f).
In main.cpp: after creating the window set GlobalWindow. Each frame: Dt = FrameClock(); if Dt > 0.1f then Dt = 0.1f. Count frames; every 0.5 seconds compute CurrentFPS and DebugSet(0, "FPS", CurrentFPS). Call DebugFlush() only every 0.25 seconds (never every frame, it is slow).
```

Check: the title shows `FPS: <number>` that updates twice a second without flicker.

## Step 3: Math (files: base.cpp, main.cpp for the test)

```
STEP 3: Math library in base.cpp. Write EXACTLY these. No CRT.

consts: PI = 3.14159265f, TAU = 6.28318531f, NEAR_W = 0.05f
f32 Abs(f32), i32 AbsI(i32), f32 Min(f32,f32), f32 Max(f32,f32), i32 MinI(i32,i32), i32 MaxI(i32,i32)
f32 Clamp(f32 v, f32 lo, f32 hi), f32 Lerp(f32 a, f32 b, f32 t) = a + (b-a)*t
f32 Floor(f32 x): i32 i = (i32)x; return (x >= (f32)i) ? (f32)i : (f32)(i-1);
i32 FloorI(f32 x): same logic returning i32.
i32 FloorDiv(i32 a, i32 b) (b > 0): a / b rounded toward negative infinity.
i32 Mod(i32 a, i32 b) (b > 0): result always in 0..b-1.
f32 Sqrt(f32 x): return 0 if x <= 0; otherwise Newton: g = (x > 1.0f) ? x*0.5f : 1.0f; repeat 24 times: g = 0.5f*(g + x/g); return g.
f32 Sin(f32 x): x = x - TAU*Floor(x/TAU + 0.5f) (now in -PI..PI); if x > PI/2: x = PI - x; if x < -PI/2: x = -PI - x; then return x - x^3/6 + x^5/120 - x^7/5040 + x^9/362880 (compute the powers explicitly with multiplications).
f32 Cos(f32 x): return Sin(x + PI*0.5f).
void MemZero(void* p, i32 bytes), void MemCopy(void* dst, const void* src, i32 bytes): simple byte loops.

struct v3 { f32 x, y, z; }; struct v4 { f32 x, y, z, w; }; struct m4 { f32 m[4][4]; };
v3 V3(f32,f32,f32); v3 V3Add(v3,v3); v3 V3Sub(v3,v3); v3 V3Scale(v3,f32); f32 V3Dot(v3,v3); v3 V3Cross(v3,v3); f32 V3Len(v3); v3 V3Norm(v3) (return the input unchanged if its length < 0.000001f)
v4 V4(f32,f32,f32,f32)
m4 M4Identity(); m4 M4Mul(m4 a, m4 b) (result = a*b, triple loop); v4 M4MulV4(m4 m, v4 v)
m4 M4Perspective(f32 fovY, f32 aspect): f = Cos(fovY*0.5f)/Sin(fovY*0.5f); all zeros except m[0][0] = f/aspect, m[1][1] = f, m[2][2] = 1, m[3][2] = 1 (so clip.w = view z).
m4 M4View(v3 pos, f32 yaw, f32 pitch): compute forward, right, up per CONVENTIONS. Rows: row0 = (right.x, right.y, right.z, -Dot(right,pos)); row1 = (up..., -Dot(up,pos)); row2 = (forward..., -Dot(forward,pos)); row3 = (0,0,0,1).

main.cpp (temporary test, each frame after the gradient), DebugSet slots 1..5, each value multiplied by 1000 and cast to i32:
 slot1 "SQRT2" = Sqrt(2.0f); slot2 "SIN" = Sin(PI/2); slot3 "COS" = Cos(PI);
 slot4 "VIEWZ" = M4MulV4(M4View(V3(0,0,0),0,0), V4(0,0,5,1)).z;
 slot5 "PROJW" = M4MulV4(M4Perspective(1.2f,1.77f), V4(0,0,5,1)).w
```

Check: the title shows SQRT2: 1414 | SIN: 1000 | COS: -1000 | VIEWZ: 5000 | PROJW: 5000.

## Step 4: Input (files: base.cpp, main.cpp)

```
STEP 4: Input.
In base.cpp add:
struct Input { bool w, a, s, d, up, down, left, right, space, ctrl, shift, escape, lmb, rmb, lmbPressed, rmbPressed, f3, f3Pressed, f5Pressed, t, f, fPressed; bool keyNum[5]; i32 mouseDX, mouseDY; };
Input In; bool MouseCaptured = false; plus statics for the previous state of lmb, rmb, f3, f5, f.
void PollInput():
 - If GetForegroundWindow() != GlobalWindow: set everything in In to false/0 and return.
 - Fill the fields with GetAsyncKeyState(vk) & 0x8000: 'W','A','S','D', VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_SPACE, VK_CONTROL, VK_SHIFT, VK_ESCAPE, VK_LBUTTON, VK_RBUTTON, '1'..'5' into keyNum[0..4], VK_F3, VK_F5, 'T', 'F'.
 - *Pressed fields are true only on the frame where the key went from up to down.
 - Mouse: if MouseCaptured: compute the client center in screen coordinates (GetClientRect + ClientToScreen), GetCursorPos, mouseDX/DY = cursor - center, then SetCursorPos(center). Otherwise mouseDX = mouseDY = 0.
 - If not captured and lmbPressed: MouseCaptured = true; ShowCursor(FALSE).
 - If captured and escape pressed (edge, use a static previous-state): MouseCaptured = false; ShowCursor(TRUE). 
main.cpp: call PollInput() each frame before drawing. ESC no longer closes the window directly: ESC while NOT captured closes it (edge-triggered), ESC while captured only releases the mouse.
Temporary test: draw a 10x10 white square (PutPixel loops) that moves with WASD at 100 pixels/second using Dt. Show mouseDX and mouseDY in title slots 1 and 2.
```

Check: the square moves in all 4 directions. Click the window: cursor hides and the title shows mouse deltas. ESC releases the cursor; ESC again quits.

---

# B. Rasterizer (test objects only, no world yet)

## Step 5: 2D triangle (files: raster.cpp new, main.cpp)

```
STEP 5: Create raster.cpp and add #include "raster.cpp" after base.cpp in main.cpp.
In raster.cpp:
struct ScreenVert { f32 x, y; };
f32 EdgeFunction(ScreenVert a, ScreenVert b, f32 px, f32 py) = (px - a.x)*(b.y - a.y) - (py - a.y)*(b.x - a.x)
struct DrawState { u32 FlatColor; }; DrawState DS;
void RasterTriangle(ScreenVert a, ScreenVert b, ScreenVert c):
 1. area = EdgeFunction(a, b, c.x, c.y). If area == 0 return. If area < 0: swap b and c and recompute area (now area > 0).
 2. Bounding box of the three vertices as floats. Clamp the float box to [0, SCREEN_W-1] x [0, SCREEN_H-1] BEFORE converting to i32 (use FloorI).
 3. For every pixel (x,y) in the box: px = x + 0.5f, py = y + 0.5f; w0 = EdgeFunction(b,c,px,py); w1 = EdgeFunction(c,a,px,py); w2 = EdgeFunction(a,b,px,py); if w0 >= 0 and w1 >= 0 and w2 >= 0: BackPixels[y*SCREEN_W + x] = DS.FlatColor.
main.cpp temporary test after the gradient: four triangles in different colors: one clockwise, the same shape mirrored (counter-clockwise), one partly off the left edge of the screen, one partly off the bottom edge.
```

Check: all four shapes fill with clean edges, both windings work, the off-screen ones are clipped, no crash.

## Step 6: Depth buffer (files: raster.cpp, main.cpp)

```
STEP 6: Depth buffer and the per-pixel shading function (raster.cpp).
- ScreenVert becomes { f32 x, y, invW; }.
- static f32 Depth[SCREEN_W*SCREEN_H]; void ClearDepth() sets every entry to 0.0f.
- u32 ShadePixel(f32 invW) { return DS.FlatColor; }   (this function will grow in later steps)
- In RasterTriangle also compute l0 = w0/area, l1 = w1/area, l2 = w2/area; invW = l0*a.invW + l1*b.invW + l2*c.invW; if invW > Depth[idx] then Depth[idx] = invW and BackPixels[idx] = ShadePixel(invW).
main.cpp test: call ClearDepth() each frame. Triangle A (red): all three vertices invW = 0.2. Triangle B (blue): same screen shape rotated/overlapping A, with invW 0.1 at one vertex and 0.4 at the other two (so it slants through A). Draw B first, then A, normally; while SPACE is held draw A first, then B.
```

Check: the same picture with and without SPACE held, with a visible diagonal line where B pokes through A.

## Step 7: 3D triangle (files: raster.cpp, main.cpp)

```
STEP 7: 3D triangle (raster.cpp).
Add:
struct Camera { v3 pos; f32 yaw; f32 pitch; }; Camera Cam = { {0.0f, 1.0f, -6.0f}, 0.0f, 0.0f }; m4 GViewProj;
void SetupCamera(): GViewProj = M4Mul(M4Perspective(1.2217f, (f32)SCREEN_W/(f32)SCREEN_H), M4View(Cam.pos, Cam.yaw, Cam.pitch));
ScreenVert ProjectToScreen(v4 clip): invW = 1.0f/clip.w; ndcX = clip.x*invW; ndcY = clip.y*invW; return { (ndcX*0.5f+0.5f)*SCREEN_W, (0.5f-ndcY*0.5f)*SCREEN_H, invW }.
void DrawTriangle3D(v3 a, v3 b, v3 c, u32 color): transform each point with M4MulV4(GViewProj, V4(p.x,p.y,p.z,1.0f)); if ANY of the three has w < NEAR_W return (clipping comes later); DS.FlatColor = color; RasterTriangle(ProjectToScreen(..), ProjectToScreen(..), ProjectToScreen(..)).
main.cpp: remove the old 2D test code. Each frame: arrow keys change Cam.yaw (left/right) and Cam.pitch (up/down, clamp +-1.5) at 1.5 rad/s * Dt; SetupCamera(); ClearScreen(MakeColor(120,170,230)); ClearDepth(); DrawTriangle3D((-2,0,0),(2,0,0),(0,3,0), orange) and DrawTriangle3D((-1,0,-1),(1,0,1),(0,2,0), cyan). Show Cam.yaw*1000 in title slot 1.
```

Check: two triangles visible. Right arrow turns the camera right (objects slide left).

## Step 8: Free camera (files: player.cpp new, main.cpp)

```
STEP 8: Free-fly camera. Create player.cpp and add its #include at the end of the include list in main.cpp.
Globals: f32 MoveSpeed = 10.0f; f32 MouseSens = 0.0025f;
void UpdateFlyCamera():
 - If MouseCaptured: Cam.yaw += In.mouseDX*MouseSens; Cam.pitch -= In.mouseDY*MouseSens; clamp pitch to +-1.5.
 - Arrow keys still turn at 1.5 rad/s * Dt (clamp pitch).
 - forwardFlat = (Sin(yaw), 0, Cos(yaw)); rightFlat = (Cos(yaw), 0, -Sin(yaw)). W/S move along forwardFlat, D/A along rightFlat, Space = up, Ctrl = down. Add the pressed directions into one vector, normalize it if its length > 0, multiply by MoveSpeed*Dt (doubled while Shift is held) and add to Cam.pos.
main.cpp: replace the old arrow-key camera code with UpdateFlyCamera() called each frame before SetupCamera(). Show Cam.pos x, y, z (x1000) in title slots 1..3.
```

Check: click to capture the mouse, look around naturally (mouse right = turn right, mouse up = look up), WASD moves relative to where you look, Space/Ctrl go up/down.

## Step 9: Near-plane clipping (files: raster.cpp, main.cpp) [risky step]

```
STEP 9: Near-plane clipping (raster.cpp). Replace the "if ANY w < NEAR_W return" rule in DrawTriangle3D.
struct ClipVert { v4 pos; f32 u; f32 v; };   (u and v are unused for now but MUST be interpolated)
i32 ClipPolygonNear(ClipVert* in, i32 inCount, ClipVert* out): Sutherland-Hodgman against the plane w = NEAR_W. Exact algorithm:
   outCount = 0
   for i in 0 .. inCount-1:
      cur = in[i]; next = in[(i+1) % inCount]
      curInside = cur.pos.w >= NEAR_W; nextInside = next.pos.w >= NEAR_W
      if curInside: out[outCount++] = cur
      if curInside != nextInside:
          t = (NEAR_W - cur.pos.w) / (next.pos.w - cur.pos.w)
          out[outCount++] = linear interpolation of EVERY field (pos.x, pos.y, pos.z, pos.w, u, v) from cur to next by t; then set its pos.w = NEAR_W exactly
   return outCount          (a triangle gives at most 4 vertices)
void DrawClipTriangle(ClipVert a, ClipVert b, ClipVert c): put them in an array of 3, call ClipPolygonNear into an array of 8, and if count >= 3 rasterize the fan: for k = 1 .. count-2: RasterTriangle(ProjectToScreen(poly[0].pos), ProjectToScreen(poly[k].pos), ProjectToScreen(poly[k+1].pos)).
DrawTriangle3D now builds three ClipVert (pos = M4MulV4(GViewProj, ...), u = v = 0) and calls DrawClipTriangle.
main.cpp test: add a huge green floor triangle (-50,-1,-50),(50,-1,-50),(0,-1,100).
```

Check: fly low over the floor, through it, and below it: no flicker, no giant garbage triangles, no hang, no crash. The floor stays visible when part of it is behind you.

## Step 10: Cubes with shading (files: raster.cpp, main.cpp)

```
STEP 10: Cubes with per-face shading and normal culling (raster.cpp).
Add these tables exactly:
const v3 FaceNormal[6] = { {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1} };
const f32 FaceCorner[6][4][3] = {   // offsets from the block's min corner
 { {1,0,0},{1,0,1},{1,1,1},{1,1,0} },   // face 0 +X
 { {0,0,1},{0,0,0},{0,1,0},{0,1,1} },   // face 1 -X
 { {0,1,0},{1,1,0},{1,1,1},{0,1,1} },   // face 2 +Y
 { {0,0,1},{1,0,1},{1,0,0},{0,0,0} },   // face 3 -Y
 { {1,0,1},{0,0,1},{0,1,1},{1,1,1} },   // face 4 +Z
 { {0,0,0},{1,0,0},{1,1,0},{0,1,0} } }; // face 5 -Z
const f32 FaceShade[6] = { 0.8f, 0.8f, 1.0f, 0.5f, 0.65f, 0.65f };
Functions:
u32 ScaleColor(u32 color, f32 s): scale r, g, b separately, clamp each to 0..255.
void DrawQuad(v3 p0, v3 p1, v3 p2, v3 p3, u32 color): DrawTriangle3D(p0,p1,p2,color); DrawTriangle3D(p0,p2,p3,color);
void DrawBlockFace(i32 x, i32 y, i32 z, i32 face, u32 baseColor): center = (x+0.5+0.5*nx, y+0.5+0.5*ny, z+0.5+0.5*nz) using FaceNormal[face]; if V3Dot(FaceNormal[face], V3Sub(Cam.pos, center)) <= 0 return; color = ScaleColor(baseColor, FaceShade[face]); DrawQuad with the four FaceCorner points added to (x,y,z).
void DrawCube(i32 x, i32 y, i32 z, u32 color): all six faces.
main.cpp test: remove the old triangles. Draw a 5x5 floor of green cubes (x,z in 0..4, y = 0) and a stack of 3 brown cubes at (2, 1..3, 2). Camera starts at (2.5, 3, -6).
```

Check: faces have different brightness (top lightest, bottom darkest), looks 3D from every side, you never see an inside face, no gaps between faces.

---

# C. Painted blocks, water, fog

## Step 11: UVs and grass/dirt painting (files: base.cpp, raster.cpp, main.cpp) [risky step]

```
STEP 11: Perspective-correct UVs and painted blocks.
base.cpp: add enum { AIR = 0, GRASS = 1, DIRT = 2, WOOD = 3, WATER = 4, LEAVES = 5 };
raster.cpp:
1. ScreenVert becomes { f32 x, y, invW, uw, vw; }. ProjectToScreen now takes a ClipVert and sets uw = cv.u*invW, vw = cv.v*invW. Update the callers (DrawClipTriangle passes the whole ClipVert).
2. RasterTriangle interpolates uw and vw with the same weights l0,l1,l2 as invW. Per pixel: u = uwInterp/invW, v = vwInterp/invW, then calls ShadePixel(invW, u, v). ShadePixel's signature becomes u32 ShadePixel(f32 invW, f32 u, f32 v).
3. DrawState becomes { u32 FlatColor; i32 Block; i32 Face; f32 Shade; }. Block == 0 means "use FlatColor".
4. DrawTriangle3D(a, b, c, color) keeps working (u = v = 0, DS.Block = 0, DS.Shade = 1).
5. New: void DrawQuadUV(v3 p0, v3 p1, v3 p2, v3 p3): corner UVs (0,0), (1,0), (1,1), (0,1); draws triangles (p0,p1,p2) and (p0,p2,p3) keeping the UVs, through DrawClipTriangle.
6. u32 Hash3(i32 a, i32 b, i32 c): u32 h = (u32)a*374761393u + (u32)b*668265263u + (u32)c*1274126177u; h = (h ^ (h >> 13)) * 1274126177u; return h ^ (h >> 16);
7. u32 PaintBlock(i32 block, i32 face, i32 pu, i32 pv): pu and pv are 0..15 (pv 15 = top edge of a side face).
   noise = Hash3(pu, pv, block*8 + face) & 15; mul = 0.92f + noise*0.01f.
   GRASS: face 2 (top) = (84,160,52); face 3 (bottom) = dirt color; faces 0,1,4,5 (sides): if pv >= 13 grass color else dirt color.
   DIRT: (134,96,67); if Hash3(pu, pv, 99) % 11 == 0 then multiply by 0.8.
   Multiply the final color by mul. Any other block: return MakeColor(255,0,255).
8. ShadePixel: if DS.Block == 0 return DS.FlatColor; else px = clamp((i32)(u*16), 0, 15), py = clamp((i32)(v*16), 0, 15); return ScaleColor(PaintBlock(DS.Block, DS.Face, px, py), DS.Shade).
9. DrawBlockFace(x, y, z, face, i32 block): same culling as before; sets DS.Block = block, DS.Face = face, DS.Shade = FaceShade[face], calls DrawQuadUV with the FaceCorner points. Remove DrawCube's color parameter (take a block id).
main.cpp test: a row of 3 cubes: grass, dirt, grass. Also one test face: a grass TOP face stretched to 8x8 units lying flat at y = -1 (call DrawQuadUV with 8-unit corners, DS.Block = GRASS, DS.Face = 2, DS.Shade = 1).
```

Check: grass cubes have a green top and dirt sides with a green strip on top; dirt is speckled brown. View the 8x8 face from a very low grazing angle: its lines stay straight and the pattern does not swim or bend.

## Step 12: Wood and leaves painting (file: raster.cpp, main.cpp)

```
STEP 12: Add WOOD and LEAVES to PaintBlock (raster.cpp). Everything is multiplied by mul as before.
 WOOD sides (faces 0,1,4,5): stripe = pu % 4: stripe 0 or 1 -> (110,86,52); stripe 2 -> (92,70,42); stripe 3 -> (120,96,60).
 WOOD top and bottom (faces 2,3): d = max(|pu-7.5|, |pv-7.5|) as f32; ring = (i32)d; if ring >= 7 -> (102,80,48); else if ring % 2 == 0 -> (176,144,92) else (146,116,72).
 LEAVES (all faces): r = Hash3(pu, pv, 40+face) % 6; r == 0 -> (26,80,26); r <= 2 -> (54,140,44); otherwise (38,110,34).
main.cpp test: a row of 4 cubes: grass, dirt, wood, leaves.
```

Check: wood has dark/light vertical bark stripes with a ring pattern on the top; leaves are speckled two-tone green. All four are recognizable from 3 blocks away.

## Step 13: Water blending (files: raster.cpp, main.cpp)

```
STEP 13: Alpha-blended water.
raster.cpp:
- DrawState gets f32 Alpha (1.0f = opaque) and bool DepthWrite. Defaults: Alpha = 1.0f, DepthWrite = true; DrawBlockFace and DrawTriangle3D must set these explicitly before drawing.
- RasterTriangle: if DS.Alpha < 1.0f: out = per-channel Lerp(existing BackPixels[idx], newColor, DS.Alpha); write only color, and write Depth only if DS.DepthWrite is true. The depth TEST (invW > Depth) still applies.
- PaintBlock: WATER returns MakeColor(48,100,210) (no mul).
- New void DrawWaterFace(i32 x, i32 y, i32 z, i32 face): same normal culling as DrawBlockFace; sets DS.Block = WATER, DS.Face = face, DS.Shade = FaceShade[face], DS.Alpha = 0.45f, DS.DepthWrite = false; DrawQuadUV; then resets DS.Alpha = 1.0f and DS.DepthWrite = true.
main.cpp test: draw all opaque stuff first, then water: a wood cube at (6,0,2), a water cube (all 6 faces with DrawWaterFace) right in front of it at (6,0,0), and a 6x6 slab of water top faces (face 2) at y = 0 over a part of the grass floor.
```

Check: the wood cube is visible through the water cube, tinted blue. The grass under the slab is visible and tinted.

## Step 14: Fog and sky (files: raster.cpp, main.cpp)

```
STEP 14: Fog and sky gradient.
raster.cpp globals: u32 SkyTopColor = MakeColor(80,140,230); u32 SkyHorizonColor = MakeColor(170,205,240); f32 FogStart = 30.0f; f32 FogEnd = 70.0f;
void DrawSky(): for each row y: t = y/(SCREEN_H-1); row color = per-channel Lerp(SkyTopColor, SkyHorizonColor, t); fill the whole row in BackPixels; then call ClearDepth().
ShadePixel: after computing the final color (including alpha cases, before returning): dist = 1.0f/invW; fog = Clamp((dist - FogStart)/(FogEnd - FogStart), 0.0f, 1.0f); return per-channel Lerp(color, SkyHorizonColor, fog).
main.cpp: use DrawSky() instead of ClearScreen + ClearDepth. Test scene: an 80x80 floor of grass TOP faces only (call DrawBlockFace(x, 0, z, 2, GRASS) for x,z in 0..79), plus the cube row. Camera starts at (40, 3, 0).
```

Check: the far floor fades smoothly into the horizon color with no visible edge where the floor ends. Show FPS: it should stay playable.

---

# D. First world

## Step 15: One chunk, drawn naively (files: world.cpp new, render.cpp new, main.cpp)

```
STEP 15: First chunk. Create world.cpp and render.cpp; in main.cpp insert their #includes after raster.cpp and BEFORE player.cpp.
world.cpp:
 const i32 CHUNK_W = 16; const i32 CHUNK_H = 64; const i32 SEA_LEVEL = 28;
 struct Chunk { u8 blocks[16][64][16]; i32 cx, cz; bool loaded; };   // index order [x][y][z]
 static Chunk TheChunk;   // one chunk at cx = 0, cz = 0
 u8 GetBlock(i32 wx, i32 wy, i32 wz): if wy < 0 return DIRT; if wy >= 64 return AIR; if wx or wz is outside 0..15 return AIR; else TheChunk.blocks[wx][wy][wz].
 void SetBlock(i32 wx, i32 wy, i32 wz, u8 id): same bounds, ignore out-of-range.
 void GenerateFlatChunk(): MemZero the blocks; y 0..7 DIRT, y 8 GRASS for every x,z; cx = cz = 0; loaded = true.
 i32 CountBlocks(): number of non-AIR blocks in TheChunk.
render.cpp:
 i32 FacesDrawn;
 void DrawChunkNaive(): FacesDrawn = 0; for every x,y,z with a non-AIR, non-WATER block, for each face f: if GetBlock(x+nx, y+ny, z+nz) == AIR: FacesDrawn++ and DrawBlockFace(x, y, z, f, block).
main.cpp: call GenerateFlatChunk() at startup. Each frame: UpdateFlyCamera, SetupCamera, DrawSky, DrawChunkNaive. Remove ALL old test drawing. Camera start (8, 14, -6). Title: CountBlocks and FacesDrawn.
```

Check: a painted 16x16 grass field. CountBlocks = 2304, FacesDrawn = 832.

## Step 16: Chunk mesh (files: world.cpp, render.cpp, main.cpp)

```
STEP 16: Chunk mesh.
world.cpp:
 struct Quad { u8 x, y, z, face, block, light; u8 ao[4]; };   // x,z = 0..15 local, y = 0..63
 const i32 MAX_QUADS = 12000; const i32 MAX_WATER_QUADS = 2000;
 Chunk gets: Quad quads[MAX_QUADS]; i32 quadCount; Quad waterQuads[MAX_WATER_QUADS]; i32 waterCount; bool dirty;
 void BuildChunkMesh(Chunk* c): quadCount = waterCount = 0; for every local x,y,z with block b != AIR and every face f: neighbor = GetBlock(c->cx*16+x+nx, y+ny, c->cz*16+z+nz).
    If b != WATER: emit into quads when neighbor is AIR or WATER. If b == WATER: emit into waterQuads when neighbor is AIR. Stop emitting when an array is full. Fill light = 255 and ao = {3,3,3,3} for now. Set dirty = false.
render.cpp:
 void DrawChunkMesh(Chunk* c): for each quad: DrawBlockFace(c->cx*16 + q.x, q.y, c->cz*16 + q.z, q.face, q.block).
 void DrawChunkWater(Chunk* c): for each water quad: DrawWaterFace(same coordinates).
 Remove DrawChunkNaive.
main.cpp: after GenerateFlatChunk call BuildChunkMesh(&TheChunk). Each frame: DrawChunkMesh then DrawChunkWater. Title: quadCount.
```

Check: the image is identical to step 15 and quadCount = 832.

## Step 17: Mesher test (files: world.cpp, main.cpp)

```
STEP 17: Test scene for the mesher. Add void GenerateTestChunk() in world.cpp: start from GenerateFlatChunk, then:
 - a 3x3 hole 2 deep: x 4..6, z 4..6, set y 7 and y 8 to AIR
 - a pillar of WOOD at x=10, z=10, y 9..13
 - one floating LEAVES block at (12, 12, 6)
 - a 4x4 pool: x 2..5, z 10..13, set y 8 to WATER
Use it instead of GenerateFlatChunk in main.cpp, then BuildChunkMesh.
```

Check: no internal faces drawn. You can see the hole walls and floor, the 6 faces of the floating leaf, the pool bottom through the water. Fly into the hole and look around. The quad count is printed.

---

# E. Terrain and chunks

## Step 18: Heightmap terrain (file: world.cpp, main.cpp)

```
STEP 18: Terrain from noise (world.cpp). Keep GenerateTestChunk.
u32 Hash2(i32 x, i32 z): u32 h = (u32)x*374761393u + (u32)z*668265263u; h = (h ^ (h >> 13)) * 1274126177u; return h ^ (h >> 16);
f32 Rand01(i32 x, i32 z): return (f32)(Hash2(x,z) & 0xFFFFFF) / 16777216.0f;
f32 ValueNoise2(f32 x, f32 z): ix = FloorI(x), iz = FloorI(z), fx = x - ix, fz = z - iz; sx = fx*fx*(3 - 2*fx); sz = fz*fz*(3 - 2*fz); v00 = Rand01(ix,iz), v10 = Rand01(ix+1,iz), v01 = Rand01(ix,iz+1), v11 = Rand01(ix+1,iz+1); return Lerp(Lerp(v00,v10,sx), Lerp(v01,v11,sx), sz);
f32 Fbm2(f32 x, f32 z): total = 0, amp = 0.5, freq = 0.015625f, norm = 0; 4 times: total += amp*ValueNoise2(x*freq, z*freq); norm += amp; amp *= 0.5f; freq *= 2.0f; return total/norm;
i32 TerrainHeight(i32 wx, i32 wz): h = 32 + (i32)((Fbm2((f32)wx, (f32)wz) - 0.5f) * 56.0f); clamp to 8..54.
void GenerateTerrainChunk(Chunk* c, i32 cx, i32 cz): c->cx = cx; c->cz = cz; MemZero the blocks; for each local x,z: wx = cx*16+x, wz = cz*16+z, h = TerrainHeight(wx,wz); y 0..h = DIRT; if h > SEA_LEVEL then block (x,h,z) = GRASS; for y = h+1 .. SEA_LEVEL: WATER. Set loaded = true, dirty = true.
main.cpp: use GenerateTerrainChunk(&TheChunk, 0, 0) instead of the test chunk. Camera start (8, 50, -10). Title: TerrainHeight(0,0) and CountBlocks.
```

Check: one hilly 16x16 piece with smooth slopes (no random spikes), and lower spots show water. The title numbers are identical on every run.

## Step 19: Chunk grid (files: world.cpp, render.cpp, main.cpp)

```
STEP 19: Chunk grid with neighbor lookups.
world.cpp:
 const i32 GRID_N = 5; static Chunk Chunks[GRID_N][GRID_N];   // index [slotZ][slotX]
 Chunk* GetChunkAt(i32 cx, i32 cz): slot = &Chunks[Mod(cz,GRID_N)][Mod(cx,GRID_N)]; return slot if slot->loaded and slot->cx == cx and slot->cz == cz, else 0.
 GetBlock(wx,wy,wz): wy < 0 -> DIRT; wy >= 64 -> AIR; chunk = GetChunkAt(FloorDiv(wx,16), FloorDiv(wz,16)); no chunk -> DIRT (solid, so no faces are made toward unloaded space); else the block at local coordinates (use Mod(wx,16), Mod(wz,16)).
 SetBlock: same lookup, ignore if no chunk; after writing set that chunk's dirty = true; if the local x is 0 or 15, or local z is 0 or 15, also set dirty on the neighbor chunk on that side (if loaded).
 void GenerateAllChunks(): for cz = -2..2, cx = -2..2: GenerateTerrainChunk(&Chunks[Mod(cz,5)][Mod(cx,5)], cx, cz); afterwards BuildChunkMesh for each.
 void RebuildDirtyChunks(): BuildChunkMesh for every loaded chunk with dirty == true.
 Delete TheChunk and everything that used it.
render.cpp: void DrawWorld(): for every loaded chunk DrawChunkMesh; then for every loaded chunk DrawChunkWater. Title: sum of quadCount.
main.cpp: GenerateAllChunks() at startup; each frame RebuildDirtyChunks() and DrawWorld(). Camera start (8, 55, -30).
```

Check: 80x80 blocks of continuous terrain with no cracks, holes or double walls at chunk borders (fly along a border and look at it). FPS is lower than before (note the number).

## Step 20: Water ordering and underwater look (files: render.cpp, main.cpp)

```
STEP 20: Water polish.
render.cpp:
 1. In DrawWorld: draw the opaque pass for all loaded chunks, then for the water pass: put the loaded chunks in an array of Chunk* (size GRID_N*GRID_N), sort by squared distance from the camera to the chunk center, FARTHEST FIRST (insertion sort), then call DrawChunkWater in that order.
 2. bool CameraUnderwater(): returns GetBlock(FloorI(Cam.pos.x), FloorI(Cam.pos.y), FloorI(Cam.pos.z)) == WATER.
 3. void ApplyUnderwaterTint(): blends every pixel of BackPixels 35% toward MakeColor(20,60,140).
main.cpp: each frame: if CameraUnderwater() set FogStart = 2.0f, FogEnd = 24.0f, else 30.0f and 70.0f. After DrawWorld, if underwater call ApplyUnderwaterTint().
```

Check: lakes are visible from the shore with the lake bed showing through. Dive in: the screen turns blue and the fog closes in; come back up and everything restores.

## Step 21: Trees (file: world.cpp)

```
STEP 21: Trees. A tree must never be cut at a chunk border.
world.cpp:
 bool TreeAt(i32 wx, i32 wz, i32* groundY): h = TerrainHeight(wx,wz); if h <= SEA_LEVEL return false; if Hash2(wx*7+13, wz*7+29) % 40 != 0 return false; *groundY = h; return true;
 void PlaceBlockLocal(Chunk* c, i32 lx, i32 ly, i32 lz, u8 id, bool onlyIfAir): ignores coordinates outside 0..15 / 0..63 / 0..15; if onlyIfAir only writes where the block is AIR.
 In GenerateTerrainChunk, after the terrain fill: for tz = -2..17, tx = -2..17 (columns up to 2 OUTSIDE the chunk): wx = cx*16+tx, wz = cz*16+tz; if TreeAt(wx, wz, &g):
    trunk: WOOD at local (tx, y, tz) for y = g+1 .. g+5 (PlaceBlockLocal, not onlyIfAir)
    leaves (onlyIfAir = true): for y = g+4 and y = g+5: every (dx,dz) with |dx| <= 2 and |dz| <= 2 except the four corners (|dx| == 2 and |dz| == 2);
                               for y = g+6: the 5 blocks (0,0), (1,0), (-1,0), (0,1), (0,-1).
    All leaf positions are (tx+dx, y, tz+dz) local.
```

Check: trees on grass only (never in water), the canopy is intact even where it crosses a chunk border (fly along the borders to check). Wood and leaves are painted.

## Step 22: Caves (file: world.cpp)

```
STEP 22: Caves.
world.cpp:
 f32 Rand01_3(i32 x, i32 y, i32 z): return (f32)(Hash3(x*3+1, y*5+2, z*7+3) & 0xFFFFFF) / 16777216.0f;   (Hash3 is in raster.cpp)
 f32 ValueNoise3(f32 x, f32 y, f32 z): like ValueNoise2 but with 8 corners and trilinear interpolation using smoothstep fractions (sx, sy, sz).
 In GenerateTerrainChunk, after the terrain fill and BEFORE the trees: for each local x,z with wx, wz, h = TerrainHeight: for y = 3 .. h-3:
    n1 = ValueNoise3(wx*0.045f, y*0.07f, wz*0.045f);
    n2 = ValueNoise3(wx*0.045f + 137.0f, y*0.07f + 59.0f, wz*0.045f + 211.0f);
    if Abs(n1 - 0.5f) < 0.045f and Abs(n2 - 0.5f) < 0.045f then set the block to AIR.
 Never carve y < 3 and never the top 2 layers of the ground.
```

Check: tunnels run through hills and open onto steep slopes. Fly along 3 slopes to find an opening. If there are no caves at all, raise both 0.045f thresholds to 0.07f. Same caves on every run.

## Step 23: Streaming (files: world.cpp, render.cpp, main.cpp)

```
STEP 23: Infinite streaming. Set GRID_N = 9 (view radius 4 chunks).
world.cpp:
 void UpdateStreaming(i32 playerWX, i32 playerWZ): pcx = FloorDiv(playerWX,16), pcz = FloorDiv(playerWZ,16); R = GRID_N/2; generated = 0; for cz = pcz-R .. pcz+R, cx = pcx-R .. pcx+R: slot = &Chunks[Mod(cz,GRID_N)][Mod(cx,GRID_N)];
    if (!slot->loaded || slot->cx != cx || slot->cz != cz): if generated >= 2 continue; GenerateTerrainChunk(slot, cx, cz); slot->dirty = true; mark the four neighbor chunks (GetChunkAt, if non-zero) dirty = true; generated++.
 RebuildDirtyChunks: rebuild at most 3 chunks per call.
 Remove GenerateAllChunks.
render.cpp: DrawWorld skips chunks that are not loaded OR whose cx/cz are farther than R chunks from the camera chunk.
main.cpp: at startup call UpdateStreaming(spawn) repeatedly (up to 60 times) until nothing was generated, and RebuildDirtyChunks() repeatedly until no chunk is dirty. Each frame: UpdateStreaming(FloorI(Cam.pos.x), FloorI(Cam.pos.z)); RebuildDirtyChunks(); DrawWorld().
```

Check: fly in one direction for 2 minutes: new terrain keeps appearing ahead, no seams, memory in Task Manager stays flat. Fly back: the same terrain returns.

## Step 24: Frustum culling and draw order (file: render.cpp)

```
STEP 24: Chunk culling and sorting.
render.cpp:
 bool ChunkVisible(Chunk* c): box min = (cx*16, 0, cz*16), max = min + (16, 64, 16). Transform the 8 corners with M4MulV4(GViewProj, V4(x,y,z,1)). For each of 5 planes count the corners outside: left x < -w; right x > w; bottom y < -w; top y > w; behind w < NEAR_W. If all 8 corners are outside the SAME plane return false, otherwise true.
 DrawWorld: collect loaded, in-window, visible chunks into an array; sort ascending by squared distance to the camera for the opaque pass (nearest first); the water pass walks the same array in REVERSE (farthest first). Remove the old separate water sort.
 Globals i32 ChunksDrawn, QuadsDrawn (sum of quadCount of the chunks drawn). Show both in the title.
```

Check: ChunksDrawn drops when you look at the sky or away from the terrain. No chunk disappears while any part of it is still on screen (look at the screen edges while turning).

---

# F. Look

## Step 25: Ambient occlusion (files: world.cpp, raster.cpp, render.cpp) [risky step]

```
STEP 25: Ambient occlusion per vertex.
world.cpp: add these tables exactly:
 const i32 FaceTU[6][3] = { {0,0,1}, {0,0,-1}, {1,0,0}, {1,0,0}, {-1,0,0}, {1,0,0} };   // direction of the quad's u axis per face
 const i32 FaceTV[6][3] = { {0,1,0}, {0,1,0}, {0,0,1}, {0,0,-1}, {0,1,0}, {0,1,0} };   // direction of the quad's v axis per face
 const i32 CornerSign[4][2] = { {-1,-1}, {1,-1}, {1,1}, {-1,1} };                       // (su, sv) of corners 0..3
 bool IsOpaque(i32 wx, i32 wy, i32 wz): block is not AIR and not WATER.
 In BuildChunkMesh, for each SOLID quad (not water) with block position (wx,wy,wz) and face f: front = (wx,wy,wz) + FaceNormal[f]. For corner k = 0..3: su,sv = CornerSign[k];
    side1  = IsOpaque(front + su*FaceTU[f])
    side2  = IsOpaque(front + sv*FaceTV[f])
    corner = IsOpaque(front + su*FaceTU[f] + sv*FaceTV[f])
    ao[k] = (side1 && side2) ? 0 : 3 - (side1 + side2 + corner)      (bools count as 0 or 1)
 Water quads keep ao = {3,3,3,3}.
raster.cpp:
 - ClipVert gets f32 br (brightness); ScreenVert gets f32 brw (br * invW); ProjectToScreen sets brw = cv.br*invW. ClipPolygonNear must interpolate br too.
 - RasterTriangle interpolates brw like uw/vw. Per pixel br = brwInterp/invW. ShadePixel becomes u32 ShadePixel(f32 invW, f32 u, f32 v, f32 br) and multiplies the final color (flat or painted) by br as well.
 - DrawTriangle3D sets br = 1.0f on all three vertices.
 - const f32 AOTable[4] = { 0.5f, 0.65f, 0.82f, 1.0f };
 - DrawQuadUV(v3 p0, v3 p1, v3 p2, v3 p3, const f32* br) with 4 brightness values. Triangle split: if br[0] + br[2] < br[1] + br[3] draw triangles (p1,p2,p3) and (p1,p3,p0), otherwise (p0,p1,p2) and (p0,p2,p3). UVs stay (0,0),(1,0),(1,1),(0,1) per corner index.
 - DrawBlockFace(x, y, z, face, block, const u8* ao): br[k] = AOTable[ao[k]]. DrawWaterFace passes br = {1,1,1,1}.
render.cpp: DrawChunkMesh passes q.ao.
```

Check: the corners where blocks meet (inside corners, under trees, in the hole) are visibly darker. There are no zig-zag diagonal artifacts on flat ground.

## Step 26: Day/night (files: raster.cpp, main.cpp)

```
STEP 26: Day/night cycle.
raster.cpp globals: f32 TimeOfDay = 0.30f; f32 Daylight = 1.0f; f32 Ambient = 1.0f;
void UpdateTime(): TimeOfDay += (Dt/240.0f) * (In.t ? 30.0f : 1.0f); if TimeOfDay >= 1 subtract 1. sunHeight = Sin(TimeOfDay*TAU). Daylight = Clamp(0.5f + 1.5f*sunHeight, 0, 1). Ambient = 0.28f + 0.72f*Daylight.
 Sky: day top (80,140,230), day horizon (170,205,240), night top (6,8,24), night horizon (14,18,40). Each frame set SkyTopColor and SkyHorizonColor as the per-channel Lerp(night, day, Daylight). (Fog already uses SkyHorizonColor.)
ShadePixel: multiply the color by Ambient BEFORE the fog blend.
main.cpp: call UpdateTime() each frame before DrawSky. Show TimeOfDay*1000 in the title.
```

Check: hold T: the sky goes from day to dark and back, the terrain darkens with it, and the fog blends into the night horizon with no bright halo at the far edge.

## Step 27: Skylight (files: world.cpp, raster.cpp, render.cpp)

```
STEP 27: Simple skylight.
world.cpp:
 Chunk gets u8 topOpaque[16][16]: y of the highest block that is not AIR and not WATER in each column (0 if none). Compute it at the end of GenerateTerrainChunk (after trees). SetBlock must recompute it for the edited column (scan from y = 63 down).
 i32 SkyHeightAt(i32 wx, i32 wz): chunk lookup; if no chunk return 0.
 In BuildChunkMesh for each solid quad: front cell (fx,fy,fz) = block + FaceNormal[face]; lit = fy > SkyHeightAt(fx,fz); q.light = lit ? 255 : 90. Water quads: light = 255.
raster.cpp: DrawBlockFace gets a final parameter u8 light: DS.Shade = FaceShade[face] * (0.38f + 0.62f*light/255.0f).
render.cpp: DrawChunkMesh passes q.light; DrawChunkWater is unchanged.
```

Check: under trees, inside tunnels and the hole are clearly darker than open ground. Surface terrain looks unchanged.

---

# G. Player

## Step 28: Walking physics (file: player.cpp, main.cpp)

```
STEP 28: Gravity and collision. Keep UpdateFlyCamera.
player.cpp:
 struct Player { v3 pos; v3 vel; bool onGround; bool flying; bool inWater; }; Player P;   // pos = feet position
 const f32 PLAYER_HALF = 0.3f; const f32 PLAYER_HEIGHT = 1.8f; const f32 EYE_HEIGHT = 1.62f;
 bool IsSolidBlock(u8 b): b != AIR && b != WATER.
 bool BoxHitsSolid(v3 pos): the box is [pos.x-HALF, pos.x+HALF] x [pos.y, pos.y+HEIGHT] x [pos.z-HALF, pos.z+HALF]. Loop over every integer cell from FloorI(min) to FloorI(max - 0.0001f) on each axis; true if any IsSolidBlock(GetBlock(...)).
 void MoveAxis(i32 axis, f32 delta): while delta != 0: step = Clamp(delta, -0.2f, 0.2f); add step to P.pos on that axis; if BoxHitsSolid(P.pos): subtract step again, set P.vel on that axis to 0, if axis == 1 and the step was negative set P.onGround = true; return. delta -= step.
 void UpdatePlayer():
   - if In.fPressed: toggle P.flying.
   - mouse look and arrow-key turning exactly like UpdateFlyCamera.
   - if P.flying: fly movement on P.pos (speed 14, Shift x2, Space up, Ctrl down) with no collision; P.vel = 0.
   - else: P.onGround = false at the start; wish = WASD on the yaw plane, normalized; speed = 5.0f (x1.6 while Shift); P.vel.x = wish.x*speed; P.vel.z = wish.z*speed; P.vel.y -= 28.0f*Dt; clamp P.vel.y >= -40; if In.space and P.onGround: P.vel.y = 9.0f (check onGround from the previous frame: keep a bool wasOnGround); then MoveAxis(0, P.vel.x*Dt); MoveAxis(2, P.vel.z*Dt); MoveAxis(1, P.vel.y*Dt).
   - Cam.pos = P.pos + (0, EYE_HEIGHT, 0).
main.cpp: replace UpdateFlyCamera() with UpdatePlayer(). Spawn: P.pos = (8, 60, 8), flying = false (you fall from the sky). Title: P.onGround as 0/1.
```

Check: you fall and land on the terrain. Walls and trees stop you. You can jump up 1 block but not 2. F toggles fly mode.

## Step 29: Swimming (file: player.cpp)

```
STEP 29: Swimming.
In UpdatePlayer (non-flying branch): P.inWater = (GetBlock(FloorI(P.pos.x), FloorI(P.pos.y + 0.4f), FloorI(P.pos.z)) == WATER), computed before the movement.
When P.inWater: horizontal speed x0.6; gravity 6.0f instead of 28.0f; clamp P.vel.y >= -4.0f; if In.space is held: P.vel.y = 4.0f (swim up, do not require onGround).
```

Check: you sink slowly in a lake, Space swims up, and you can climb out at the shore by holding Space and W.

## Step 30: Block targeting (files: player.cpp, raster.cpp, hud.cpp new, main.cpp)

```
STEP 30: Block targeting with an outline.
player.cpp:
 struct Target { bool hit; i32 x, y, z; i32 px, py, pz; }; Target Tgt;   // (x,y,z) = hit block, (px,py,pz) = the empty cell before it
 void PickBlock(): Amanatides-Woo voxel walk. origin = Cam.pos; dir = camera forward (from CONVENTIONS, unit length); cell[a] = FloorI(origin[a]); step[a] = dir[a] > 0 ? 1 : -1; tDelta[a] = (dir[a] == 0) ? 1e30f : Abs(1.0f/dir[a]); tMax[a] = (dir[a] == 0) ? 1e30f : ((dir[a] > 0 ? (cell[a] + 1 - origin[a]) : (origin[a] - cell[a])) * tDelta[a]).
   Tgt.hit = false. Loop at most 64 times:
      axis = index of the smallest tMax; if tMax[axis] > 6.0f return;
      prev = cell; cell[axis] += step[axis]; tMax[axis] += tDelta[axis];
      if IsSolidBlock(GetBlock(cell)): fill Tgt (hit = true, cell, prev); return.
raster.cpp:
 void DrawLine3D(v3 a, v3 b, u32 color): transform both ends with GViewProj; if both w < NEAR_W return; if one is behind, move it onto the plane w = NEAR_W by linear interpolation (all of x,y,z,w); project to screen; step along the longer screen axis one pixel at a time interpolating invW; draw only if the pixel is inside the screen and invW >= Depth[idx]*0.99f; write the color, do NOT write depth.
hud.cpp (new, include last): 
 void FillRect(i32 x, i32 y, i32 w, i32 h, u32 color)  (clips to the screen)
 void DrawCrosshair(): a white plus (arms 4 pixels long) at the screen center
 void DrawBlockOutline(i32 x, i32 y, i32 z): the 12 edges of the block box expanded by 0.002 on each side, color black.
main.cpp: each frame after DrawWorld: PickBlock(); if Tgt.hit DrawBlockOutline(Tgt.x,Tgt.y,Tgt.z); DrawCrosshair(). Title: Tgt.x, Tgt.y, Tgt.z and the block id there (GetBlock).
```

Check: a black outline hugs the block under the crosshair, it hides correctly behind nearer blocks, reach is 6 blocks, the title coordinates match what you are looking at.

## Step 31: Break and place (files: player.cpp, world.cpp)

```
STEP 31: Break and place blocks.
player.cpp globals: u8 HotbarBlocks[5] = { GRASS, DIRT, WOOD, LEAVES, WATER }; u8 Selected = GRASS.
In UpdatePlayer, only when MouseCaptured:
 - keys 1..5 (In.keyNum[i]) set Selected = HotbarBlocks[i].
 - if In.lmbPressed and Tgt.hit: SetBlock(Tgt.x,Tgt.y,Tgt.z, AIR).
 - if In.rmbPressed and Tgt.hit: cell = (Tgt.px,Tgt.py,Tgt.pz); the cell must currently be AIR or WATER; if Selected is not WATER, the player box (BoxHitsSolid logic) must not overlap that cell: test by temporarily checking if the cell's AABB intersects [P.pos.x-0.3, P.pos.x+0.3] x [P.pos.y, P.pos.y+1.8] x [P.pos.z-0.3, P.pos.z+0.3]. Then SetBlock(cell, Selected).
world.cpp: SetBlock already marks dirty chunks; make sure that if the edited block's local x or z is 0 or 15 the neighbor chunk on that side is also marked dirty.
main.cpp: RebuildDirtyChunks may rebuild up to 4 chunks per frame.
```

Check: break a dirt block and the hole appears with its inner faces. Place wood on top. Place water. You cannot place a block inside yourself. Keys 1-5 switch. Edit a block on a chunk border: both chunks update, no seam.

## Step 32: HUD and text overlay (file: hud.cpp, main.cpp)

```
STEP 32: HUD (hud.cpp).
 const u32 BlockAvgColor[6] = { 0, MakeColor(84,160,52), MakeColor(134,96,67), MakeColor(130,104,64), MakeColor(48,100,210), MakeColor(40,115,36) };
 void DrawHotbar(): 5 squares of 22x22 pixels with a 4 pixel gap, centered horizontally, 6 pixels above the bottom. Fill each with BlockAvgColor[HotbarBlocks[i]] and a 1 px border 0x202020; the selected one gets a 2 px white border drawn outside it.
 Font: 5x7 pixels, each glyph = 7 bytes, one per row from top to bottom, bit 4 = leftmost pixel. Add this table EXACTLY:
 static const u8 FontGlyphs[26][7] = {
  {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, // 0
  {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, // 1
  {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}, // 2
  {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E}, // 3
  {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, // 4
  {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}, // 5
  {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, // 6
  {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, // 7
  {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, // 8
  {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}, // 9
  {0x00,0x00,0x00,0x1F,0x00,0x00,0x00}, // -
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // space
  {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}, // A
  {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}, // C
  {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}, // D
  {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}, // F
  {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}, // H
  {0x11,0x12,0x14,0x18,0x14,0x12,0x11}, // K
  {0x11,0x19,0x15,0x13,0x11,0x11,0x11}, // N
  {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}, // P
  {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}, // Q
  {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}, // S
  {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}, // U
  {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}, // X
  {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}, // Y
  {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}  // Z
 };
 Glyph index: '0'..'9' = 0..9, '-' = 10, ' ' = 11, then A=12, C=13, D=14, F=15, H=16, K=17, N=18, P=19, Q=20, S=21, U=22, X=23, Y=24, Z=25. Any other character draws nothing.
 void DrawText(i32 x, i32 y, const char* text, u32 color): 6 px per character; draws a dark shadow (offset +1,+1) first.
 i32 DrawNumber(i32 x, i32 y, i32 value, u32 color): handles 0 and negatives, converts the digits itself, draws with DrawText, returns the x after the last digit.
 bool ShowOverlay = false; void DrawDebugOverlay(): when ShowOverlay: lines at x = 4 and y = 4, 14, 24, 34: "FPS n" (CurrentFPS), "XYZ x y z" (feet position, integers), "CHUNK cx cz", "QUADS n" (QuadsDrawn). Build each line with DrawText for the label and DrawNumber for each value.
main.cpp: each frame after the crosshair: DrawHotbar(); if In.f3Pressed toggle ShowOverlay; DrawDebugOverlay().
```

Check: the hotbar shows 5 colored squares with the selected one outlined. F3 shows readable text lines with live numbers.

---

# H. Scale

## Step 33: Worker thread for chunks (files: world.cpp, main.cpp) [risky step]

```
STEP 33: Background chunk generation with ONE worker thread. 
world.cpp:
 - Chunk: replace `bool loaded` with `volatile i32 state` (0 = EMPTY, 1 = WORKING, 2 = READY) and add `volatile i32 needsNeighborDirty`. Everywhere the code tested `loaded`, test `state == 2` instead. GetChunkAt returns a chunk only if state == 2 and cx/cz match. GenerateTerrainChunk no longer sets loaded. RebuildDirtyChunks only processes chunks with state == 2.
 - struct ChunkJob { i32 slotX, slotZ, cx, cz; }; static ChunkJob JobQueue[128]; static volatile LONG JobHead = 0; static volatile LONG JobTail = 0; static volatile LONG WorkerQuit = 0;
 - bool PushJob(ChunkJob job) (main thread only): next = (JobTail+1) % 128; if next == JobHead return false; JobQueue[JobTail] = job; MemoryBarrier(); JobTail = next; return true;
 - DWORD WINAPI WorkerMain(LPVOID param): while WorkerQuit == 0: if JobHead == JobTail { Sleep(1); continue; } job = JobQueue[JobHead]; slot = &Chunks[job.slotZ][job.slotX]; GenerateTerrainChunk(slot, job.cx, job.cz); BuildChunkMesh(slot); slot->needsNeighborDirty = 1; MemoryBarrier(); slot->state = 2; MemoryBarrier(); JobHead = (JobHead+1) % 128;
 - UpdateStreaming (main thread), for each wanted (cx,cz) in the window: slot = ...; if slot->state == 1 continue; if the slot is not (state == 2 with the same cx,cz): set slot->state = 1 first (the old chunk becomes unusable immediately) then PushJob({slotX, slotZ, cx, cz}); if PushJob fails set state = 0 and stop the loop. At most 4 PushJob calls per UpdateStreaming call.
 - At the end of UpdateStreaming (main thread): for every slot with state == 2 and needsNeighborDirty == 1: set needsNeighborDirty = 0 and mark the four neighbor chunks (GetChunkAt, if non-zero) dirty = true.
main.cpp: CreateThread(0, 0, WorkerMain, 0, 0, 0) once at startup. Before the first frame: call UpdateStreaming(spawn position) in a loop with Sleep(1) until the 9 chunks around the spawn have state == 2 and none of them is dirty (RebuildDirtyChunks inside the loop). Set WorkerQuit = 1 when the window closes.
```

Check: no hitch when you cross a chunk border while flying fast. FPS stays steady. Chunks appear at the horizon a moment late (expected). No crash after 5 minutes of flying around. No holes at chunk borders once things settle.

## Step 34: Multithreaded rasterizer (files: raster.cpp, render.cpp, main.cpp) [risky step]

```
STEP 34: Rasterize the screen in horizontal bands, one thread per band.
raster.cpp:
 1. struct RasterTri { ScreenVert v[3]; i32 block, face; f32 shade; f32 alpha; bool depthWrite; u32 flatColor; };
    const i32 MAX_TRIS = 150000; static RasterTri OpaqueTris[MAX_TRIS]; static RasterTri WaterTris[MAX_TRIS]; i32 OpaqueCount, WaterCount;
 2. RasterTriangle(a,b,c) no longer draws: it makes a RasterTri from a, b, c and the current DS and appends it to WaterTris if DS.Alpha < 1.0f, else to OpaqueTris (skip when the array is full). Do the "area < 0 swap" and the "area == 0 skip" there, once.
 3. New void RasterizeTriBand(const RasterTri* t, i32 y0, i32 y1): the old pixel loop, with the y range clamped to [y0, y1) and all state (block, face, shade, alpha, depthWrite, flatColor) read from t instead of DS. ShadePixel gets the same state as extra parameters instead of reading DS.
 4. New void RasterizeBand(i32 y0, i32 y1): draws the sky for rows y0..y1-1 and clears the depth for those rows (the old DrawSky logic restricted to those rows), then loops all OpaqueTris, then all WaterTris, calling RasterizeTriBand.
 5. Threads: N = number of CPU cores (GetSystemInfo, dwNumberOfProcessors, clamped 1..8). Band k covers rows k*SCREEN_H/N to (k+1)*SCREEN_H/N. For k = 1..N-1 create a worker thread with HANDLE StartEvent[k] and DoneEvent[k] (CreateEventA, auto-reset, initially unset). Worker k: loop { WaitForSingleObject(StartEvent[k], INFINITE); RasterizeBand(its rows); SetEvent(DoneEvent[k]); }.
    void RenderBands(): SetEvent for all StartEvents; RasterizeBand for band 0 on the calling thread; WaitForMultipleObjects(N-1, &DoneEvent[1], TRUE, INFINITE) (skip when N == 1).
render.cpp: DrawWorld no longer calls DrawSky.
main.cpp frame order: SetupCamera/UpdateTime; OpaqueCount = 0; WaterCount = 0; DrawWorld (only collects triangles); RenderBands(); then, on the main thread, everything that draws directly into BackPixels: underwater tint, block outline, crosshair, hotbar, text. Show N and OpaqueCount in the title.
```

Check: the picture is identical to before, with no flicker or lines at the band borders. FPS goes up noticeably on a multi-core CPU. Closing the window exits cleanly.

## Step 35: Save and load edits (files: world.cpp, player.cpp, main.cpp)

```
STEP 35: Persist player edits.
world.cpp:
 struct Edit { i32 x, y, z; u8 block; }; static Edit Edits[200000]; static volatile i32 EditCount = 0;
 void PlayerSetBlock(i32 x, i32 y, i32 z, u8 id): calls SetBlock, then records the edit: search the last 64 entries for the same x,y,z and overwrite it; otherwise append (ignore if full).
 In GenerateTerrainChunk, at the very end (after trees and caves): read count = EditCount once into a local; for i in 0..count-1: if the edit falls inside this chunk, write its block with PlaceBlockLocal. Then compute topOpaque (it must come after this).
 void SaveEdits(): CreateFileA("world.bin", GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0); WriteFile the i32 EditCount then the used part of Edits; CloseHandle.
 void LoadEdits(): if the file exists read it back into EditCount/Edits. Call it at startup BEFORE the first UpdateStreaming.
player.cpp: break/place now call PlayerSetBlock.
main.cpp: F5 (In.f5Pressed) calls SaveEdits() and DebugSet(7, "SAVED", EditCount). Also call SaveEdits() when the window closes.
```

Check: break and place some blocks, travel far enough that the area unloads, come back: the edits are still there. Press F5, quit, restart: the edits are still there.

## Step 36 (optional): Greedy meshing (files: world.cpp, raster.cpp, render.cpp)

```
STEP 36: Greedy meshing for solid blocks (water quads stay single blocks).
world.cpp:
 const i32 FaceUAxis[6] = {2,2,0,0,0,0}; const i32 FaceVAxis[6] = {1,1,2,2,1,1}; const i32 FaceNAxis[6] = {0,0,1,1,2,2};   // 0 = x, 1 = y, 2 = z
 Quad gets u8 w, h (single-block quads have w = h = 1).
 Replace the solid-quad part of BuildChunkMesh: for each face f, for each layer n along axis FaceNAxis[f] (size 16 for x or z, 64 for y):
   1. Build a mask over (u, v) (u along FaceUAxis[f], v along FaceVAxis[f]). For each cell find the block at the local coordinates; if it is solid and its neighbor in direction FaceNormal[f] is AIR or WATER, then key = 1 | (block << 1) | (light << 4) | (ao[0] << 12) | (ao[1] << 14) | (ao[2] << 16) | (ao[3] << 18) (light and ao are computed exactly as in steps 25 and 27); otherwise key = 0.
   2. Greedy merge: scan v then u. For the first cell with key != 0 that is not yet used: width w = count of consecutive cells in u with the same key (not used); then height h = number of following rows in which ALL w cells have the same key and are unused. Emit ONE Quad at the min cell with w, h; mark the rectangle used.
raster.cpp: DrawBlockFace(x, y, z, face, block, ao, light, w, h): for corner k, position along each axis = block position + FaceCorner[face][k][axis] + (if FaceCorner[face][k][axis] == 1: (axis == FaceUAxis[face] ? w-1 : axis == FaceVAxis[face] ? h-1 : 0)). Corner UVs become (0,0), (w,0), (w,h), (0,h). The normal culling uses the center of the merged rectangle.
 ShadePixel: pu = ((i32)(u*16.0f)) & 15 and pv = ((i32)(v*16.0f)) & 15 (wrap, so the 16x16 pattern repeats once per block).
render.cpp: DrawChunkMesh passes q.w and q.h.
```

Check: the image is identical to before (AO and light are preserved, patterns tile once per block), and QuadsDrawn drops a lot on flat ground.

---

## When Qwen misbehaves

- It rewrote a whole file or dropped functions: add "Output ONLY the changed functions" at the end of the prompt and paste the RULES again at the end too.
- It used std::, printf, new or any other header: the RULES did not stick. Put them at the start AND at the end of the prompt.
- It renamed things: add "Use the exact names from API.txt" to the prompt.
- It changed things you did not ask for: switch to the FIX prompt and name a single problem.
- It cannot finish a step: split the step in half (do the first half of the list, check, then the second half). The riskiest steps are 9, 11, 25, 33 and 34. For 33 and 34 paste only the files they name.
