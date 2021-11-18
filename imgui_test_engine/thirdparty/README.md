## Third party libraries used by Test Engine

Always used:
- `Str/Str.h` simple string type, used by `imgui_test_engine`

Used if `IMGUI_TEST_ENGINE_ENABLE_IMPLOT` is defined:
- `implot/` plotting library, used by `imgui_test_engine`

Used unless `IMGUI_TEST_ENGINE_DISABLE_CAPTURE` is defined:
- `gif-h/` gif writer, used by `imgui_capture_tool`
- `stb/stb_image_write.h` image writer, used by `imgui_capture_tool`
