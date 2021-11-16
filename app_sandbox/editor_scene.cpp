#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include <math.h>
#include "../libs/ImGuizmo/ImGuizmo.h"
#include "../shared/IconsFontAwesome5.h"
#include "editor_widgets.h"

// Sample adapted from https://github.com/CedricGuillemet/ImGuizmo/blob/master/example/main.cpp
// In theory we could have a ImVec3 with maths ops + ImMatrix44 here..
// But our goal is to demonstrate ImGuizmo with as little code as possible, so the maths primitives are a little ugy...

static void Frustum(float left, float right, float bottom, float top, float znear, float zfar, float mtx[4*4])
{
    float temp, temp2, temp3, temp4;
    temp = 2.0f * znear;
    temp2 = right - left;
    temp3 = top - bottom;
    temp4 = zfar - znear;
    mtx[0] = temp / temp2;
    mtx[1] = 0.0;
    mtx[2] = 0.0;
    mtx[3] = 0.0;
    mtx[4] = 0.0;
    mtx[5] = temp / temp3;
    mtx[6] = 0.0;
    mtx[7] = 0.0;
    mtx[8] = (right + left) / temp2;
    mtx[9] = (top + bottom) / temp3;
    mtx[10] = (-zfar - znear) / temp4;
    mtx[11] = -1.0f;
    mtx[12] = 0.0;
    mtx[13] = 0.0;
    mtx[14] = (-temp * zfar) / temp4;
    mtx[15] = 0.0;
}

static void Perspective(float fovyInDegrees, float aspectRatio, float znear, float zfar, float mtx[4*4])
{
    float ymax, xmax;
    ymax = znear * tanf(fovyInDegrees * 3.141592f / 180.0f);
    xmax = ymax * aspectRatio;
    Frustum(-xmax, xmax, -ymax, ymax, znear, zfar, mtx);
}

static void Cross(const float* a, const float* b, float* r)
{
    r[0] = a[1] * b[2] - a[2] * b[1];
    r[1] = a[2] * b[0] - a[0] * b[2];
    r[2] = a[0] * b[1] - a[1] * b[0];
}

static float Dot(const float* a, const float* b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static void Normalize(const float* a, float* r)
{
    float il = 1.f / (sqrtf(Dot(a, a)) + FLT_EPSILON);
    r[0] = a[0] * il;
    r[1] = a[1] * il;
    r[2] = a[2] * il;
}

static void LookAt(const float* eye, const float* at, const float* up, float mtx[4*4])
{
    float X[3], Y[3], Z[3], tmp[3];

    tmp[0] = eye[0] - at[0];
    tmp[1] = eye[1] - at[1];
    tmp[2] = eye[2] - at[2];
    Normalize(tmp, Z);
    Normalize(up, Y);

    Cross(Y, Z, tmp);
    Normalize(tmp, X);

    Cross(Z, X, tmp);
    //tmp.cross(Z, X);
    Normalize(tmp, Y);
    //Y.normalize(tmp);

    mtx[0] = X[0];
    mtx[1] = Y[0];
    mtx[2] = Z[0];
    mtx[3] = 0.0f;
    mtx[4] = X[1];
    mtx[5] = Y[1];
    mtx[6] = Z[1];
    mtx[7] = 0.0f;
    mtx[8] = X[2];
    mtx[9] = Y[2];
    mtx[10] = Z[2];
    mtx[11] = 0.0f;
    mtx[12] = -Dot(X, eye);
    mtx[13] = -Dot(Y, eye);
    mtx[14] = -Dot(Z, eye);
    mtx[15] = 1.0f;
}

static void Orthographic(const float l, float r, float b, const float t, float zn, const float zf, float mtx[4*4])
{
    mtx[0] = 2 / (r - l);
    mtx[1] = 0.0f;
    mtx[2] = 0.0f;
    mtx[3] = 0.0f;
    mtx[4] = 0.0f;
    mtx[5] = 2 / (t - b);
    mtx[6] = 0.0f;
    mtx[7] = 0.0f;
    mtx[8] = 0.0f;
    mtx[9] = 0.0f;
    mtx[10] = 1.0f / (zf - zn);
    mtx[11] = 0.0f;
    mtx[12] = (l + r) / (l - r);
    mtx[13] = (t + b) / (b - t);
    mtx[14] = zn / (zn - zf);
    mtx[15] = 1.0f;
}

struct EditorDemoScene
{
    // Methods
    void    Render();
    void    ManipulateObject(float mtx[4*4]);
    void    LookAt(float x, float y, float z);

    // Members
    float ObjectMatrix[4*4] =
    {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
    float CameraView[4*4] =
    {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    };

    // Members
    bool                IsPerspective = true;
    float               FOV = 27.f;
    float               ViewWidth = 10.f;
    ImVec2              CameraAngle = ImVec2(32.f / 180.f * 3.14159f, 165.f / 180.f * 3.14159f);
    float               CameraDistance = 8.f;
    float               CameraProjection[4*4];
    ImGuizmo::OPERATION GizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE      GizmoMode = ImGuizmo::WORLD;
};

void EditorDemoScene::Render()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImVec2 pos = ImGui::GetWindowPos() + ImVec2(style.WindowPadding.x, style.WindowPadding.y + window->TitleBarHeight());           // FIXME-SANDBOX: ClientPos/StartPos
    ImVec2 size = ImGui::GetWindowSize() - ImVec2(style.WindowPadding.x * 2, style.WindowPadding.y * 2 + window->TitleBarHeight()); // FIXME-SANDBOX: ClientRect
    if (size.x <= 0.0f || size.y <= 0.0f)
        return;

    if (IsPerspective)
    {
        Perspective(FOV, size.x / size.y, 0.1f, 100.f, CameraProjection);
    }
    else
    {
        float viewHeight = ViewWidth * size.y / size.x;
        Orthographic(-ViewWidth, ViewWidth, -viewHeight, viewHeight, 1000.f, -1000.f, CameraProjection);
    }
    ImGuizmo::SetOrthographic(!IsPerspective);
    ImGuizmo::BeginFrame();
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());

    ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y);

    // Draw sample scene
    // (ImGuizmo has helpers functions to draw a 3D grid using ImDrawList. In a real engine it is likely that the majority
    // your of display will be rendered using your engine functions and not via ImDrawList)
    const float IdentityMatrix[4*4] =
    {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
    ImGuizmo::DrawGrid(CameraView, CameraProjection, IdentityMatrix, 100.f);
    ImGuizmo::DrawCubes(CameraView, CameraProjection, &ObjectMatrix[0], 1);

    // Toolbar buttons
    ImGui::SetCursorScreenPos(pos + style.FramePadding);

    ImGui::BeginButtonGroup();
    if (ImGui::ToggleButton(ICON_FA_ARROWS_ALT, "Translate", GizmoOperation == ImGuizmo::TRANSLATE))
        GizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::ToggleButton(ICON_FA_SYNC, "Rotate", GizmoOperation == ImGuizmo::ROTATE))
        GizmoOperation = ImGuizmo::ROTATE;
    if (ImGui::ToggleButton(ICON_FA_EXPAND_ARROWS_ALT, "Scale", GizmoOperation == ImGuizmo::SCALE))
        GizmoOperation = ImGuizmo::SCALE;
    ImGui::EndButtonGroup();

    ImGui::SameLine(0, 3.0f);

    ImGui::BeginButtonGroup();
    if (ImGui::ToggleButton(ICON_FA_ARROWS_ALT, "World", GizmoMode == ImGuizmo::WORLD))
        GizmoMode = ImGuizmo::WORLD;
    if (ImGui::ToggleButton(ICON_FA_EXPAND_ARROWS_ALT, "Local", GizmoMode == ImGuizmo::LOCAL))
        GizmoMode = ImGuizmo::LOCAL;
    ImGui::EndButtonGroup();

    ImGui::SameLine(0, 3.f);

    ImGui::BeginButtonGroup();
    if (ImGui::ToggleButton(ICON_FA_CUBES, "Perspective", IsPerspective))
        IsPerspective = true;
    if (ImGui::ToggleButton(ICON_FA_BORDER_ALL, "Orthographic", !IsPerspective))
        IsPerspective = false;
    ImGui::EndButtonGroup();

    // Cube manipulator
    ImVec2 cube_size = ImVec2(128.0f, 128.0f);
    ImVec2 cube_pos = ImVec2(pos.x + size.x - cube_size.x, pos.y);
    ImGui::SetCursorScreenPos(cube_pos);
    ImGui::InvisibleButton("##cube", cube_size);
    ImGuizmo::ViewManipulate(CameraView, CameraDistance, cube_pos, cube_size, 0x10101010);

    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton("##scene", size);

    ManipulateObject(ObjectMatrix); // TODO: Make independent
}

void EditorDemoScene::ManipulateObject(float mtx[4*4])
{
    // Gizmo manipulator
    // ImGuizmo::CanActivate() was modified so gizmo works when we have clicked InvisibleButton.
    ImGuizmo::Manipulate(CameraView, CameraProjection, GizmoOperation, GizmoOperation == ImGuizmo::SCALE ? ImGuizmo::LOCAL : GizmoMode, mtx);
}

void EditorDemoScene::LookAt(float x, float y, float z)
{
    float eye[] =
    {
        cosf(CameraAngle.y) * cosf(CameraAngle.x) * CameraDistance,
        sinf(CameraAngle.x) * CameraDistance,
        sinf(CameraAngle.y) * cosf(CameraAngle.x) * CameraDistance
    };
    float at[] = { x, y, z };
    float up[] = { 0.f, 1.f, 0.f };
    ::LookAt(eye, at, up, CameraView);
}

void EditorRenderScene()
{
    static EditorDemoScene* scene = NULL;
    if (scene == NULL)
    {
        scene = IM_NEW(EditorDemoScene);
        scene->LookAt(0.0f, 0.0f, 0.0f);
    }
    scene->Render();
}
