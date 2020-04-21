#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#

EXE = imgui_test_app
IMGUI_PATH = ../../imgui
SOURCES = main.cpp
# FIXME: The paths look wrong here??
SOURCES += ../imgui_tests.cpp ../imgui_te_context.cpp ../imgui_te_core.cpp ../imgui_te_coroutine.cpp ../imgui_te_ui.cpp ../imgui_te_util.cpp
SOURCES += $(IMGUI_PATH)/imgui.cpp $(IMGUI_PATH)/imgui_demo.cpp $(IMGUI_PATH)/imgui_draw.cpp $(IMGUI_PATH)/imgui_widgets.cpp
SOURCES += ../shared/imgui_app.cpp ../shared/imgui_capture_tool.cpp ../shared/imgui_utils.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)

CXXFLAGS = -std=c++11
CFLAGS = -I../ -I$(IMGUI_PATH) -I$(IMGUI_PATH)/examples
CFLAGS += -DIMGUI_USER_CONFIG=\"test_app/test_app_imconfig.h\"
CFLAGS += -g -Wall -Wformat

ifeq ($(BACKEND_LIB),)
	ifeq ($(shell pkg-config --exists sdl2 && echo 1), 1)
		BACKEND_LIB := sdl2
	endif
endif

ifeq ($(BACKEND_LIB),)
	ifeq ($(shell pkg-config --exists glfw3 && echo 1), 1)
		BACKEND_LIB := glfw3
	endif
endif

ifneq ($(BACKEND_LIB),)
	ifeq ($(BACKEND_LIB), sdl2)
		CFLAGS += -DIMGUI_APP_SDL_GL3
		SOURCES += $(IMGUI_PATH)/examples/imgui_impl_sdl.cpp
	endif
	ifeq ($(BACKEND_LIB), glfw3)
		CFLAGS += -DIMGUI_APP_GLFW_GL3
		SOURCES += $(IMGUI_PATH)/examples/imgui_impl_glfw.cpp
	endif
	LIBS = $(shell pkg-config --libs $(BACKEND_LIB))
	CFLAGS += $(shell pkg-config --cflags $(BACKEND_LIB))
	CFLAGS += -I$(IMGUI_PATH)/examples/libs/gl3w
	CFLAGS += -DIMGUI_IMPL_OPENGL_LOADER_GL3W
	SOURCES += $(IMGUI_PATH)/examples/imgui_impl_opengl3.cpp $(IMGUI_PATH)/examples/libs/gl3w/GL/gl3w.c
endif

LIBS += -lpthread

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += -ldl
	ifneq ($(BACKEND_LIB),)
		LIBS += -lGL
	endif
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework CoreFoundation
	ifeq ($(HAS_SDL2), 1)
		LIBS += -framework OpenGL
	endif
endif

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
	ECHO_MESSAGE = "MinGW"
	ifeq ($(HAS_SDL2), 1)
		LIBS += -lopengl32
	endif
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:%.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:../test_engine/%.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:../shared/%.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:../libs/Str/%.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:../../imgui/%.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:../../imgui/examples/%.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:../../imgui/examples/libs/gl3w/GL/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
