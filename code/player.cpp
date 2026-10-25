// player.cpp - free-fly camera movement. Included last by main.cpp, so it may use
// Camera/Cam (raster.cpp) and In/MouseCaptured/Dt/math/debug helpers (base.cpp).

f32 MoveSpeed = 10.0f;
f32 MouseSens = 0.0025f;

const f32 CAMERA_TURN_SPEED = 1.5f; // radians per second, arrow keys

void UpdateFlyCamera()
{
    // Mouse look only exists while the cursor is captured. Moving the mouse right turns
    // the camera right; moving it up (mouseDY is negative) looks up.
    if (MouseCaptured)
    {
        Cam.yaw += (f32)In.mouseDX * MouseSens;
        Cam.pitch -= (f32)In.mouseDY * MouseSens;
    }

    if (In.left) Cam.yaw -= CAMERA_TURN_SPEED * Dt;
    if (In.right) Cam.yaw += CAMERA_TURN_SPEED * Dt;
    if (In.up) Cam.pitch += CAMERA_TURN_SPEED * Dt;
    if (In.down) Cam.pitch -= CAMERA_TURN_SPEED * Dt;

    Cam.pitch = Clamp(Cam.pitch, -1.5f, 1.5f);

    // Movement runs on the flat XZ plane, so looking up or down does not change the speed.
    v3 forwardFlat = V3(Sin(Cam.yaw), 0.0f, Cos(Cam.yaw));
    v3 rightFlat = V3(Cos(Cam.yaw), 0.0f, -Sin(Cam.yaw));

    v3 wish = V3(0.0f, 0.0f, 0.0f);
    if (In.w) wish = V3Add(wish, forwardFlat);
    if (In.s) wish = V3Sub(wish, forwardFlat);
    if (In.d) wish = V3Add(wish, rightFlat);
    if (In.a) wish = V3Sub(wish, rightFlat);
    if (In.space) wish = V3Add(wish, V3(0.0f, 1.0f, 0.0f));
    if (In.ctrl) wish = V3Sub(wish, V3(0.0f, 1.0f, 0.0f));

    f32 speed = MoveSpeed;
    if (In.shift)
    {
        speed *= 2.0f;
    }

    // A diagonal or up+forward wish is normalised, so it does not move faster than one axis.
    if (V3Len(wish) > 0.0f)
    {
        v3 move = V3Scale(V3Norm(wish), speed * Dt);
        Cam.pos = V3Add(Cam.pos, move);
    }

    DebugSet(1, "X", i32(Cam.pos.x * 1000.0f));
    DebugSet(2, "Y", i32(Cam.pos.y * 1000.0f));
    DebugSet(3, "Z", i32(Cam.pos.z * 1000.0f));

    // ANALYSIS ONLY, removed after the step 10 check: the spec asks for pos only, but the four
    // azimuth dumps and the shading checks need yaw/pitch visible to be measurable.
    DebugSet(4, "YAW", i32(Cam.yaw * 1000.0f));
    DebugSet(5, "PITCH", i32(Cam.pitch * 1000.0f));
}
