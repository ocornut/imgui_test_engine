#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include <math.h>
#include "../libs/ImGuizmo/ImGuizmo.h"
#include "../shared/IconsFontAwesome5.h"
#include "editor_widgets.h"

// Sample adapted from https://github.com/CedricGuillemet/ImGuizmo/blob/master/example/main.cpp

static void Frustum(float left, float right, float bottom, float top, float znear, float zfar, float *m16)
{
    float temp, temp2, temp3, temp4;
    temp = 2.0f * znear;
    temp2 = right - left;
    temp3 = top - bottom;
    temp4 = zfar - znear;
    m16[0] = temp / temp2;
    m16[1] = 0.0;
    m16[2] = 0.0;
    m16[3] = 0.0;
    m16[4] = 0.0;
    m16[5] = temp / temp3;
    m16[6] = 0.0;
    m16[7] = 0.0;
    m16[8] = (right + left) / temp2;
    m16[9] = (top + bottom) / temp3;
    m16[10] = (-zfar - znear) / temp4;
    m16[11] = -1.0f;
    m16[12] = 0.0;
    m16[13] = 0.0;
    m16[14] = (-temp * zfar) / temp4;
    m16[15] = 0.0;
}

static void Perspective(float fovyInDegrees, float aspectRatio, float znear, float zfar, float *m16)
{
    float ymax, xmax;
    ymax = znear * tanf(fovyInDegrees * 3.141592f / 180.0f);
    xmax = ymax * aspectRatio;
    Frustum(-xmax, xmax, -ymax, ymax, znear, zfar, m16);
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

static void Normalize(const float* a, float *r)
{
    float il = 1.f / (sqrtf(Dot(a, a)) + FLT_EPSILON);
    r[0] = a[0] * il;
    r[1] = a[1] * il;
    r[2] = a[2] * il;
}

static void LookAt(const float* eye, const float* at, const float* up, float *m16)
{
    float X[3], Y[3], Z[3], tmp[3];

    tmp[0] = eye[0] - at[0];
    tmp[1] = eye[1] - at[1];
    tmp[2] = eye[2] - at[2];
    //Z.normalize(eye - at);
    Normalize(tmp, Z);
    Normalize(up, Y);
    //Y.normalize(up);

    Cross(Y, Z, tmp);
    //tmp.cross(Y, Z);
    Normalize(tmp, X);
    //X.normalize(tmp);

    Cross(Z, X, tmp);
    //tmp.cross(Z, X);
    Normalize(tmp, Y);
    //Y.normalize(tmp);

    m16[0] = X[0];
    m16[1] = Y[0];
    m16[2] = Z[0];
    m16[3] = 0.0f;
    m16[4] = X[1];
    m16[5] = Y[1];
    m16[6] = Z[1];
    m16[7] = 0.0f;
    m16[8] = X[2];
    m16[9] = Y[2];
    m16[10] = Z[2];
    m16[11] = 0.0f;
    m16[12] = -Dot(X, eye);
    m16[13] = -Dot(Y, eye);
    m16[14] = -Dot(Z, eye);
    m16[15] = 1.0f;
}

static void Orthographic(const float l, float r, float b, const float t, float zn, const float zf, float *m16)
{
    m16[0] = 2 / (r - l);
    m16[1] = 0.0f;
    m16[2] = 0.0f;
    m16[3] = 0.0f;
    m16[4] = 0.0f;
    m16[5] = 2 / (t - b);
    m16[6] = 0.0f;
    m16[7] = 0.0f;
    m16[8] = 0.0f;
    m16[9] = 0.0f;
    m16[10] = 1.0f / (zf - zn);
    m16[11] = 0.0f;
    m16[12] = (l + r) / (l - r);
    m16[13] = (t + b) / (b - t);
    m16[14] = zn / (zn - zf);
    m16[15] = 1.0f;
}

struct EditorDemoScene
{
    void Render();
    void ManipulateObject(float* matrix);
    void LookAt(float x, float y, float z);

    float ObjectMatrix[16] = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
    const float IdentityMatrix[16] = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
    float CameraView[16] = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
    bool IsPerspective = true;
    float FOV = 27.f;
    float ViewWidth = 10.f;
    ImVec2 CameraAngle = ImVec2(32.f / 180.f * 3.14159f, 165.f / 180.f * 3.14159f);
    float CameraDistance = 8.f;
    float CameraProjection[16];
    ImGuizmo::OPERATION GizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE GizmoMode = ImGuizmo::WORLD;
};

void EditorDemoScene::Render()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImVec2 pos = ImGui::GetWindowPos() + ImVec2(style.WindowPadding.x, style.WindowPadding.y + window->TitleBarHeight()); // FIXME: ClientPos/StartPos
    ImVec2 size = ImGui::GetWindowSize() - ImVec2(style.WindowPadding.x * 2, style.WindowPadding.y * 2 + window->TitleBarHeight()); // FIXME: ClientRect
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
    ImGuizmo::SetDrawlist();

    ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y);

    // Draw sample scene
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

void EditorDemoScene::ManipulateObject(float* matrix)
{
    // Gizmo manipulator
    // ImGuizmo::CanActivate() was modified so gizmo works when we have clicked InvisibleButton.
    ImGuizmo::Manipulate(CameraView, CameraProjection, GizmoOperation, GizmoOperation == ImGuizmo::SCALE ? ImGuizmo::LOCAL : GizmoMode, matrix);
}

void EditorDemoScene::LookAt(float x, float y, float z)
{
    float eye[] = {
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
