// dear imgui
// (test engine, core)

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "imgui_te_exporters.h"
#include "imgui_te_engine.h"
#include "imgui_te_internal.h"
#include "thirdparty/Str/Str.h"

//-------------------------------------------------------------------------
// [SECTION] FORWARD DECLARATION
//-------------------------------------------------------------------------

void ImGuiTestEngine_ExportJUnitXml(ImGuiTestEngine* engine, const char* output_file);

//-------------------------------------------------------------------------
// [SECTION] TEST ENGINE EXPORTER FUNCTIONS
//-------------------------------------------------------------------------
// - ImGuiTestEngine_Export()
// - ImGuiTestEngine_ExportJUnitXml()
//-------------------------------------------------------------------------

void ImGuiTestEngine_Export(ImGuiTestEngine* engine)
{
    ImGuiTestEngineIO& io = engine->IO;
    if (io.ExportResultsFormat == ImGuiTestEngineExportFormat_None)
        return;
    IM_ASSERT(io.ExportResultsFile != NULL);

    if (io.ExportResultsFormat == ImGuiTestEngineExportFormat_JUnitXml)
        ImGuiTestEngine_ExportJUnitXml(engine, io.ExportResultsFile);
    else
        IM_ASSERT(0);
}

void ImGuiTestEngine_ExportJUnitXml(ImGuiTestEngine* engine, const char* output_file)
{
    IM_ASSERT(engine != NULL);
    IM_ASSERT(output_file != NULL);

    FILE* fp = fopen(output_file, "w+b");
    if (fp == NULL)
    {
        fprintf(stderr, "Writing '%s' failed.\n", output_file);
        return;
    }

    // Per-testsuite test statistics.
    struct
    {
        const char* Name     = NULL;
        int         Tests    = 0;
        int         Failures = 0;
        int         Disabled = 0;
    } testsuites[ImGuiTestGroup_COUNT];
    testsuites[ImGuiTestGroup_Tests].Name = "tests";
    testsuites[ImGuiTestGroup_Perfs].Name = "perfs";

    for (int n = 0; n < engine->TestsAll.Size; n++)
    {
        ImGuiTest* test = engine->TestsAll[n];
        auto* stats = &testsuites[test->Group];
        stats->Tests += 1;
        if (test->Status == ImGuiTestStatus_Error)
            stats->Failures += 1;
        else if (test->Status == ImGuiTestStatus_Unknown)
            stats->Disabled += 1;
    }

    // Attributes for <testsuites> tag.
    const char* testsuites_name = "Dear ImGui";
    int testsuites_failures = 0;
    int testsuites_tests = 0;
    int testsuites_disabled = 0;
    float testsuites_time = (float)((double)(engine->EndTime - engine->StartTime) / 1000000.0);
    for (int testsuite_id = ImGuiTestGroup_Tests; testsuite_id < ImGuiTestGroup_COUNT; testsuite_id++)
    {
        testsuites_tests += testsuites[testsuite_id].Tests;
        testsuites_failures += testsuites[testsuite_id].Failures;
        testsuites_disabled += testsuites[testsuite_id].Disabled;
    }

    // FIXME: "errors" attribute and <error> tag in <testcase> may be supported if we have means to catch unexpected errors like assertions.
    fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<testsuites disabled=\"%d\" errors=\"0\" failures=\"%d\" name=\"%s\" tests=\"%d\" time=\"%.3f\">\n",
        testsuites_disabled, testsuites_failures, testsuites_name, testsuites_tests, testsuites_time);

    const char* teststatus_names[] = { "skipped", "success", "queued", "running", "error", "suspended" };
    for (int testsuite_id = ImGuiTestGroup_Tests; testsuite_id < ImGuiTestGroup_COUNT; testsuite_id++)
    {
        // Attributes for <testsuite> tag.
        auto* testsuite = &testsuites[testsuite_id];
        float testsuite_time = testsuites_time;         // FIXME: We do not differentiate between tests and perfs, they are executed in one big batch.
        Str30 testsuite_timestamp = "";
        ImTimestampToISO8601(engine->StartTime, &testsuite_timestamp);
        fprintf(fp, "  <testsuite name=\"%s\" tests=\"%d\" disabled=\"%d\" errors=\"0\" failures=\"%d\" hostname=\"\" id=\"%d\" package=\"\" skipped=\"0\" time=\"%.3f\" timestamp=\"%s\">\n",
            testsuite->Name, testsuite->Tests, testsuite->Disabled, testsuite->Failures, testsuite_id, testsuite_time, testsuite_timestamp.c_str());

        for (int n = 0; n < engine->TestsAll.Size; n++)
        {
            ImGuiTest* test = engine->TestsAll[n];
            if (test->Group != testsuite_id)
                continue;

            // Attributes for <testcase> tag.
            const char* testcase_name = test->Name;
            const char* testcase_classname = test->Category;
            const char* testcase_status = teststatus_names[test->Status + 1];   // +1 because _Unknown status is -1.
            float testcase_time = (float)((double)(test->EndTime - test->StartTime) / 1000000.0);

            fprintf(fp, "    <testcase name=\"%s\" assertions=\"0\" classname=\"%s\" status=\"%s\" time=\"%.3f\"",
                testcase_name, testcase_classname, testcase_status, testcase_time);

            if (test->Status == ImGuiTestStatus_Error)
            {
                // Skip last error message because it is generic information that test failed.
                Str128 log_line;
                for (int i = test->TestLog.LineInfoError.Size - 2; i >= 0; i--)
                {
                    ImGuiTestLogLineInfo* line_info = &test->TestLog.LineInfoError[i];
                    if (line_info->Level == ImGuiTestVerboseLevel_Error)
                    {
                        const char* line_start = test->TestLog.Buffer.c_str() + line_info->LineOffset;
                        const char* line_end = strstr(line_start, "\n");
                        log_line.set(line_start, line_end);
                        ImStrXmlEscape(&log_line);
                        break;
                    }
                }

                fprintf(fp, ">\n"); // End of <testcase> tag
                fprintf(fp, "      <failure message=\"%s\" type=\"error\">\n", log_line.c_str());

                // Save full log in text element of <failure> tag.
                for (auto& line_info : test->TestLog.LineInfoError)
                {
                    const char* line_start = test->TestLog.Buffer.c_str() + line_info.LineOffset;
                    const char* line_end = strstr(line_start, "\n");
                    log_line.set(line_start, line_end);
                    ImStrXmlEscape(&log_line);
                    fprintf(fp, "        %s\n", log_line.c_str());
                }

                fprintf(fp, "      </failure>\n");
                fprintf(fp, "    </testcase>\n");
            }
            else
            {
                fprintf(fp, "/>\n");    // Terminate <testcase>
            }
        }

        // FIXME: These could be supported by capturing stdout/stderr, or printing all log messages.
        fprintf(fp, "    <system-out></system-out>\n");
        fprintf(fp, "    <system-err></system-err>\n");
        fprintf(fp, "  </testsuite>\n");
    }
    fprintf(fp, "</testsuites>\n");
    fclose(fp);
}
