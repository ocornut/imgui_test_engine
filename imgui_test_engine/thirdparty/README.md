## Third party libraries used by Test Engine

Always used:
- `Str/Str.h` simple string type, used by `imgui_test_engine`

Used if `IMGUI_TEST_ENGINE_ENABLE_IMPLOT` is defined to 1 (default: 0)
- `implot/` plotting library, used by `imgui_test_engine`

Used if `IMGUI_TEST_ENGINE_ENABLE_CAPTURE` is defined to 1 (default: 1)
- `gif-h/` gif writer, used by `imgui_capture_tool`
- `stb/imstb_image_write.h` image writer, used by `imgui_capture_tool`
