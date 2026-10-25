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

// --- Near-plane clipping (step 9) ---
// u and v are unused for now, but ClipPolygonNear must interpolate them so the UVs
// survive the cut when step 11 starts painting blocks.
struct ClipVert
{
    v4 pos;
    f32 u;
    f32 v;
};

v4 LerpV4(v4 a, v4 b, f32 t)
{
    return V4(Lerp(a.x, b.x, t), Lerp(a.y, b.y, t), Lerp(a.z, b.z, t), Lerp(a.w, b.w, t));
}

// Sutherland-Hodgman against the plane w = NEAR_W. A triangle comes out with at most 4 vertices.
i32 ClipPolygonNear(ClipVert* in, i32 inCount, ClipVert* out)
{
    i32 outCount = 0;

    for (i32 i = 0; i < inCount; ++i)
    {
        ClipVert cur = in[i];
        ClipVert next = in[(i + 1) % inCount];

        bool curInside = cur.pos.w >= NEAR_W;
        bool nextInside = next.pos.w >= NEAR_W;

        if (curInside)
        {
            out[outCount] = cur;
            ++outCount;
        }

        if (curInside != nextInside)
        {
            // w changes linearly along the segment, so two vertices on opposite sides of the
            // plane always have different w: the denominator is never zero here.
            f32 t = (NEAR_W - cur.pos.w) / (next.pos.w - cur.pos.w);

            ClipVert hit;
            hit.pos = LerpV4(cur.pos, next.pos, t);
            hit.pos.w = NEAR_W; // the cut vertex sits exactly on the near plane
            hit.u = Lerp(cur.u, next.u, t);
            hit.v = Lerp(cur.v, next.v, t);

            out[outCount] = hit;
            ++outCount;
        }
    }

    return outCount;
}

void DrawClipTriangle(ClipVert a, ClipVert b, ClipVert c)
{
    ClipVert poly[3];
    poly[0] = a;
    poly[1] = b;
    poly[2] = c;

    ClipVert clipped[8];
    i32 count = ClipPolygonNear(poly, 3, clipped);
    if (count < 3) return;

    // Near-plane clipping keeps the polygon convex, so a fan from vertex 0 covers it.
    for (i32 k = 1; k <= count - 2; ++k)
    {
        ScreenVert screenA = ProjectToScreen(clipped[0].pos);
        ScreenVert screenB = ProjectToScreen(clipped[k].pos);
        ScreenVert screenC = ProjectToScreen(clipped[k + 1].pos);
        RasterTriangle(screenA, screenB, screenC);
    }
}

void DrawTriangle3D(v3 a, v3 b, v3 c, u32 color)
{
    ClipVert vertA;
    vertA.pos = M4MulV4(GViewProj, V4(a.x, a.y, a.z, 1.0f));
    vertA.u = 0.0f;
    vertA.v = 0.0f;

    ClipVert vertB;
    vertB.pos = M4MulV4(GViewProj, V4(b.x, b.y, b.z, 1.0f));
    vertB.u = 0.0f;
    vertB.v = 0.0f;

    ClipVert vertC;
    vertC.pos = M4MulV4(GViewProj, V4(c.x, c.y, c.z, 1.0f));
    vertC.u = 0.0f;
    vertC.v = 0.0f;

    DS.FlatColor = color;
    DrawClipTriangle(vertA, vertB, vertC);
}

// --- Block faces (step 10) ---
// FaceCorner holds offsets from the block's min corner, so a block at (x, y, z) occupies
// x..x+1, y..y+1, z..z+1. Winding matches RasterTriangle's counter-clockwise rule.
const v3 FaceNormal[6] =
{
    { 1.0f, 0.0f, 0.0f },
    { -1.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f },
    { 0.0f, -1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, -1.0f }
};

const f32 FaceCorner[6][4][3] =
{
    { { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 0.0f } },   // face 0 +X
    { { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 1.0f } },   // face 1 -X
    { { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 1.0f } },   // face 2 +Y
    { { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },   // face 3 -Y
    { { 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f } },   // face 4 +Z
    { { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } }    // face 5 -Z
};

const f32 FaceShade[6] = { 0.8f, 0.8f, 1.0f, 0.5f, 0.65f, 0.65f };

u32 ScaleColor(u32 color, f32 s)
{
    u32 r = (color >> 16) & 0xFFu;
    u32 g = (color >> 8) & 0xFFu;
    u32 b = color & 0xFFu;

    // Each channel is scaled on its own and clamped, so a shade can never push a channel past 255.
    i32 scaledR = i32(f32(r) * s);
    i32 scaledG = i32(f32(g) * s);
    i32 scaledB = i32(f32(b) * s);

    return MakeColor(u8(MaxI(0, MinI(255, scaledR))), u8(MaxI(0, MinI(255, scaledG))), u8(MaxI(0, MinI(255, scaledB))));
}

void DrawQuad(v3 p0, v3 p1, v3 p2, v3 p3, u32 color)
{
    DrawTriangle3D(p0, p1, p2, color);
    DrawTriangle3D(p0, p2, p3, color);
}

void DrawBlockFace(i32 x, i32 y, i32 z, i32 face, u32 baseColor)
{
    v3 normal = FaceNormal[face];

    // The face centre sits half a unit out along the normal. A face only draws when the camera
    // is on the side the normal points to, which is what keeps inside faces hidden.
    v3 center = V3(f32(x) + 0.5f + 0.5f * normal.x,
                   f32(y) + 0.5f + 0.5f * normal.y,
                   f32(z) + 0.5f + 0.5f * normal.z);

    if (V3Dot(normal, V3Sub(Cam.pos, center)) <= 0.0f) return;

    u32 color = ScaleColor(baseColor, FaceShade[face]);

    v3 corner0 = V3(f32(x) + FaceCorner[face][0][0], f32(y) + FaceCorner[face][0][1], f32(z) + FaceCorner[face][0][2]);
    v3 corner1 = V3(f32(x) + FaceCorner[face][1][0], f32(y) + FaceCorner[face][1][1], f32(z) + FaceCorner[face][1][2]);
    v3 corner2 = V3(f32(x) + FaceCorner[face][2][0], f32(y) + FaceCorner[face][2][1], f32(z) + FaceCorner[face][2][2]);
    v3 corner3 = V3(f32(x) + FaceCorner[face][3][0], f32(y) + FaceCorner[face][3][1], f32(z) + FaceCorner[face][3][2]);

    DrawQuad(corner0, corner1, corner2, corner3, color);
}

void DrawCube(i32 x, i32 y, i32 z, u32 color)
{
    for (i32 face = 0; face < 6; ++face)
    {
        DrawBlockFace(x, y, z, face, color);
    }
}
