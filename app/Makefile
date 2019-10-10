#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#

EXE = imgui_test_app
SOURCES = main.cpp
SOURCES += ../imgui_te_core.cpp ../imgui_te_util.cpp ../imgui_tests.cpp ../imgui_te_context.cpp
SOURCES += ../../imgui/imgui.cpp ../../imgui/imgui_demo.cpp ../../imgui/imgui_draw.cpp ../../imgui/imgui_widgets.cpp
SOURCES += ../../imgui/examples/imgui_impl_opengl3.cpp ../../imgui/examples/imgui_impl_sdl.cpp ../../imgui/examples/libs/gl3w/GL/gl3w.c
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)

CXXFLAGS = -std=c++11
CFLAGS = -I../ -I../../imgui -I../../imgui/examples -I../../imgui/examples/libs/gl3w
CFLAGS += -DIMGUI_ENABLE_TEST_ENGINE -DIMGUI_TESTS_BACKEND_SDL_GL3 -DIMGUI_IMPL_OPENGL_LOADER_GL3W
CFLAGS += -g -Wall -Wformat `pkg-config --cflags sdl2`
LIBS = `pkg-config --libs sdl2`

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += -ldl -lGL
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework CoreFoundation -framework OpenGL
endif

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
	ECHO_MESSAGE = "MinGW"
	LIBS += -lopengl32
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:%.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:../%.cpp
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
