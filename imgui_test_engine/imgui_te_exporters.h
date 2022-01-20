// dear imgui
// (test engine, core)

#pragma once

//-------------------------------------------------------------------------
// Forward Declarations
//-------------------------------------------------------------------------

struct ImGuiTestEngine;

//-------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------

enum ImGuiTestEngineExportFormat_
{
    ImGuiTestEngineExportFormat_None = 0,
    ImGuiTestEngineExportFormat_JUnitXml,           // JUnit XML format described at https://llg.cubic.org/docs/junit/.
};
typedef int ImGuiTestEngineExportFormat;

//-------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------

void ImGuiTestEngine_Export(ImGuiTestEngine* engine);
