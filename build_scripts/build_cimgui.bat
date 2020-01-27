@echo off
git clone https://github.com/cimgui/cimgui.git cimgui
rmdir /Q cimgui\imgui
git clone https://github.com/ocornut/imgui.git cimgui\imgui
mkdir cimgui\cmake-build
pushd cimgui\cmake-build
cmake -DCIMGUI_TEST=1 ..
cmake --build . -- /p:Configuration=Release
Release\cimgui_test.exe
popd
rmdir /Q /S cimgui
del /F /Q imgui.ini
@pause
