#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui_utils.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "../imgui_test_engine/thirdparty/Str/Str.h"

#if defined(_WIN32)
#if !defined(_WINDOWS_)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <shellapi.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#endif
#if defined(__linux) || defined(__linux__) || defined(__MACH__) || defined(__MSL__)
#include <pthread.h>    // pthread_setname_np()
#endif
#include <chrono>
#include <thread>

