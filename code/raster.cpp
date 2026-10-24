// raster.cpp - triangle rasterizer for the low-resolution backbuffer.
// Only uses things from base.cpp (BackPixels, PutPixel, SCREEN_W, SCREEN_H, MakeColor,
// Min, Max, FloorI) and types declared earlier in this file.

// windows.h defines DrawState as a macro for its button-drawing API (DrawStateA/W).
// We never use that API, and the spec names our struct DrawState, so the macro is dropped.
#undef DrawState

struct ScreenVert
{
    f32 x, y, invW;
};

f32 EdgeFunction(ScreenVert a, ScreenVert b, f32 px, f32 py)
{
    return (px - a.x) * (b.y - a.y) - (py - a.y) * (b.x - a.x);
}

struct DrawState
{
    u32 FlatColor;
};

DrawState DS;

// Depth buffer: one entry per backbuffer pixel. Bigger invW means closer to the camera.
static f32 Depth[SCREEN_W * SCREEN_H];

void ClearDepth()
{
    for (i32 i = 0; i < SCREEN_W * SCREEN_H; ++i)
    {
        Depth[i] = 0.0f;
    }
}

// This function grows in later steps (block shading, water alpha). For now it is flat colour.
u32 ShadePixel(f32 invW)
{
    return DS.FlatColor;
}

void RasterTriangle(ScreenVert a, ScreenVert b, ScreenVert c)
{
    // Step 1 of the spec: reject degenerate triangles and make the winding positive.
    f32 area = EdgeFunction(a, b, c.x, c.y);
    if (area == 0.0f) return;
    if (area < 0.0f)
    {
        ScreenVert swappedB = b;
        b = c;
        c = swappedB;
        area = EdgeFunction(a, b, c.x, c.y);
    }

    // Step 2: bounding box of the three vertices as floats, clamped in float space
    // before converting to integers, so off-screen triangles are clipped.
    f32 leftFloat = Min(a.x, Min(b.x, c.x));
    f32 rightFloat = Max(a.x, Max(b.x, c.x));
    f32 topFloat = Min(a.y, Min(b.y, c.y));
    f32 bottomFloat = Max(a.y, Max(b.y, c.y));

    if (leftFloat < 0.0f) leftFloat = 0.0f;
    if (topFloat < 0.0f) topFloat = 0.0f;
    if (rightFloat > (f32)(SCREEN_W - 1)) rightFloat = (f32)(SCREEN_W - 1);
    if (bottomFloat > (f32)(SCREEN_H - 1)) bottomFloat = (f32)(SCREEN_H - 1);

    i32 left = FloorI(leftFloat);
    i32 right = FloorI(rightFloat);
    i32 top = FloorI(topFloat);
    i32 bottom = FloorI(bottomFloat);

    // Step 3: test the pixel centre against all three edges.
    for (i32 y = top; y <= bottom; ++y)
    {
        for (i32 x = left; x <= right; ++x)
        {
            f32 px = (f32)x + 0.5f;
            f32 py = (f32)y + 0.5f;

            f32 w0 = EdgeFunction(b, c, px, py);
            f32 w1 = EdgeFunction(c, a, px, py);
            f32 w2 = EdgeFunction(a, b, px, py);

            if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f)
            {
                i32 index = y * SCREEN_W + x;

                f32 l0 = w0 / area;
                f32 l1 = w1 / area;
                f32 l2 = w2 / area;
                f32 invW = l0 * a.invW + l1 * b.invW + l2 * c.invW;

                if (invW > Depth[index])
                {
                    Depth[index] = invW;
                    BackPixels[index] = ShadePixel(invW);
                }
            }
        }
    }
}

// --- Camera and 3D projection (step 7) ---
struct Camera
{
    v3 pos;
    f32 yaw;
    f32 pitch;
};

Camera Cam = { {0.0f, 1.0f, -6.0f}, 0.0f, 0.0f };
m4 GViewProj;

void SetupCamera()
{
    m4 projection = M4Perspective(1.2217f, (f32)SCREEN_W / (f32)SCREEN_H);
    m4 view = M4View(Cam.pos, Cam.yaw, Cam.pitch);
    GViewProj = M4Mul(projection, view);
}

ScreenVert ProjectToScreen(v4 clip)
{
    f32 invW = 1.0f / clip.w;
    f32 ndcX = clip.x * invW;
    f32 ndcY = clip.y * invW;

    ScreenVert result;
    result.x = (ndcX * 0.5f + 0.5f) * (f32)SCREEN_W;
    result.y = (0.5f - ndcY * 0.5f) * (f32)SCREEN_H;
    result.invW = invW;
    return result;
}

void DrawTriangle3D(v3 a, v3 b, v3 c, u32 color)
{
    v4 clipA = M4MulV4(GViewProj, V4(a.x, a.y, a.z, 1.0f));
    v4 clipB = M4MulV4(GViewProj, V4(b.x, b.y, b.z, 1.0f));
    v4 clipC = M4MulV4(GViewProj, V4(c.x, c.y, c.z, 1.0f));

    // Any vertex behind the near plane rejects the whole triangle. Real clipping is step 9,
    // so the test scene stays entirely in front of the camera.
    if (clipA.w < NEAR_W || clipB.w < NEAR_W || clipC.w < NEAR_W) return;

    DS.FlatColor = color;
    RasterTriangle(ProjectToScreen(clipA), ProjectToScreen(clipB), ProjectToScreen(clipC));
}
