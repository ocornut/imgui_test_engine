## Contents

- `app_mimimal` minimal demo app showcasing how to integrate the test engine (app)
- `app_sandbox/` experiments, showcase of third party extensions (app)
- `build_scripts/` helpers scripts to build with various compilers and create packages.
- `shared/` shared C++ helpers for apps
- `imgui_tests/` test suite (app)
- `imgui_test_engine/` test engine (library)


## build_scripts/

Note: those are merely helpers to build Dear ImGui with variety of compilers without relying on CI workflows.
For day to day operation, use the provided Solution file.
For local testing with Clang, MinGW etc: copy [build_scripts/config/default_build_config.bat](https://github.com/ocornut/imgui_dev/blob/main/build_scripts/config/default_build_config.bat) to `build_config.bat` and configure your paths.
