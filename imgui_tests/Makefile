#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#

EXE = imgui_tests
IMGUI_DIR = ../../imgui
SOURCES = main.cpp
# FIXME: The paths look wrong here??
SOURCES += ../imgui_tests.cpp ../imgui_tests_docking.cpp ../imgui_tests_nav.cpp ../imgui_tests_perf.cpp ../imgui_tests_tables.cpp ../imgui_tests_viewports.cpp ../imgui_tests_widgets.cpp
SOURCES += ../imgui_te_context.cpp ../imgui_te_coroutine.cpp ../imgui_te_engine.cpp ../imgui_te_perftool.cpp ../imgui_te_ui.cpp ../imgui_te_util.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += ../shared/imgui_app.cpp ../shared/imgui_capture_tool.cpp ../shared/imgui_utils.cpp
SOURCES += ../libs/implot/implot.cpp ../libs/implot/implot_items.cpp ../libs/implot/implot_demo.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)

CXXFLAGS = -std=c++14
CFLAGS = -I../ -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I../libs
CFLAGS += -DIMGUI_USER_CONFIG=\"imgui_tests/imgui_tests_imconfig.h\"
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
		SOURCES += $(IMGUI_DIR)/backends/imgui_impl_sdl.cpp
	endif
	ifeq ($(BACKEND_LIB), glfw3)
		CFLAGS += -DIMGUI_APP_GLFW_GL3
		SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp
	endif
	LIBS = $(shell pkg-config --libs $(BACKEND_LIB))
	CFLAGS += $(shell pkg-config --cflags $(BACKEND_LIB))
	SOURCES += $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
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
	CFLAGS += -Wextra -Wpedantic
	CXXFLAGS += -Wno-class-memaccess
	ifeq ($(shell $(CXX) -v 2>&1 | grep -c "clang version"), 1)
		CFLAGS += -Wshadow
		LIBS += -lstdc++ -lm
	endif
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework CoreFoundation
	ifeq ($(HAS_SDL2), 1)
		LIBS += -framework OpenGL
	endif
endif

ifeq ($(ASAN),1)
	CFLAGS += -g -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls
	LIBS += -g -fsanitize=address
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

%.o:../libs/implot/%.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
