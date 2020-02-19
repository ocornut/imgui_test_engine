#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#

EXE = imgui_test_app
IMGUI_PATH = ../../imgui
SOURCES = main.cpp
SOURCES += ../imgui_capture_tool.cpp ../imgui_te_core.cpp ../imgui_te_util.cpp ../imgui_tests.cpp ../imgui_te_context.cpp ../backends.cpp
SOURCES += $(IMGUI_PATH)/imgui.cpp $(IMGUI_PATH)/imgui_demo.cpp $(IMGUI_PATH)/imgui_draw.cpp $(IMGUI_PATH)/imgui_widgets.cpp
SOURCES += ../libs/Str/Str.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)

CXXFLAGS = -std=c++11
CFLAGS = -I../ -I$(IMGUI_PATH) -I$(IMGUI_PATH)/examples -I../test_engine -I../libs
CFLAGS += -DIMGUI_ENABLE_TEST_ENGINE
CFLAGS += -g -Wall -Wformat
HAS_SDL2 = $(shell pkg-config --exists sdl2 && echo 1)

ifeq ($(HAS_SDL2), 1)
	LIBS = $(shell pkg-config --libs sdl2)
	CFLAGS += $(shell pkg-config --cflags sdl2)
	CFLAGS += -I$(IMGUI_PATH)/examples/libs/gl3w
	CFLAGS += -DIMGUI_TESTS_BACKEND_SDL_GL3 -DIMGUI_IMPL_OPENGL_LOADER_GL3W
	SOURCES += $(IMGUI_PATH)/examples/imgui_impl_opengl3.cpp $(IMGUI_PATH)/examples/imgui_impl_sdl.cpp $(IMGUI_PATH)/examples/libs/gl3w/GL/gl3w.c
endif

LIBS += -lpthread

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += -ldl
	ifeq ($(HAS_SDL2), 1)
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
