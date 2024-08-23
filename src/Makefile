# run "make help" for usage details

.DEFAULT_GOAL := help
.PHONY: help release debug clean

build_flags = -std=c++20 -Wall -Wextra -Werror
cpp_files = $(wildcard *.cpp)
CPP = g++


# ---------------------------------  WINDOWS  ---------------------------------
ifeq ($(OS), Windows_NT)


VERSION := $(shell for /F "tokens=3" %%i in ('findstr /C:"#define VERSION" main.cpp') do @echo %%i)
exe_release := deepblunder_$(VERSION)_mingw64.exe
exe_debug := deepblunder_debug_$(VERSION)_mingw64.exe
release_dir := build\deepblunder_release
debug_dir := build\deepblunder_debug

# Create names for object and dependency files by prepending the correct path (either
# build/release or build/debug) and appending the correct extension (either .o or .d)
release_obj_files = $(patsubst %.cpp, $(release_dir)\\%.o, $(cpp_files))
release_dep_files = $(patsubst %.cpp, $(release_dir)\\%.d, $(cpp_files))
debug_obj_files = $(patsubst %.cpp, $(debug_dir)\\%.o, $(cpp_files))
debug_dep_files = $(patsubst %.cpp, $(debug_dir)\\%.d, $(cpp_files))

# Release target
release: build_flags += -Ofast -DNDEBUG
release: $(release_obj_files)
	$(CPP) $(release_obj_files) -o $(exe_release) $(build_flags) -flto -s
-include $(release_dep_files)
$(release_dir)\\%.o: %.cpp | $(release_dir)
	$(CPP) -c $< -o $@ $(build_flags) -MMD -MF $(@:.o=.d)
$(release_dir)\\%.d: # prevent catchall target from catching .d files

# Debug target
debug: build_flags += -Og -g -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC
debug: $(debug_obj_files)
	$(CPP) $(debug_obj_files) -o $(exe_debug) $(build_flags)
-include $(debug_dep_files)
$(debug_dir)\\%.o: %.cpp | $(debug_dir)
	$(CPP) -c $< -o $@ $(build_flags) -MMD -MF $(@:.o=.d)
$(debug_dir)\\%.d: # prevent catchall target from catching .d files

# Clean target
clean:
	if exist $(exe_release) del $(exe_release)
	if exist $(exe_debug) del $(exe_debug)
	if exist build rmdir /S /Q build
	@echo Done: build directory and executables removed

# Create build directories if they don't exist
$(release_dir) $(debug_dir):
	if not exist $@ mkdir $@


# ---------------------------------  LINUX  ---------------------------------
else


VERSION := $(shell grep '#define VERSION' main.cpp | awk '{print $$3}')
exe_release := deepblunder_$(VERSION)_linux
exe_debug := deepblunder_debug_$(VERSION)_linux
release_dir := build/deepblunder_release
debug_dir := build/deepblunder_debug
build_flags += -march=native

# Create names for object and dependency files by prepending the correct path (either
# build/release or build/debug) and appending the correct extension (either .o or .d)
release_obj_files = $(patsubst %.cpp, $(release_dir)/%.o, $(cpp_files))
release_dep_files = $(patsubst %.cpp, $(release_dir)/%.d, $(cpp_files))
debug_obj_files = $(patsubst %.cpp, $(debug_dir)/%.o, $(cpp_files))
debug_dep_files = $(patsubst %.cpp, $(debug_dir)/%.d, $(cpp_files))

# Release target
release: build_flags += -Ofast -DNDEBUG
release: $(release_obj_files)
	$(CPP) $(release_obj_files) -o $(exe_release) $(build_flags) -flto -s
-include $(release_dep_files)
$(release_dir)/%.o: %.cpp | $(release_dir)
	$(CPP) -c $< -o $@ $(build_flags) -MMD -MF $(@:.o=.d)
$(release_dir)/%.d: # prevent catchall target from catching .d files

# Debug target
debug: build_flags += -Og -g -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=address -fsanitize=undefined -fno-sanitize-recover
debug: $(debug_obj_files)
	$(CPP) $(debug_obj_files) -o $(exe_debug) $(build_flags)
-include $(debug_dep_files)
$(debug_dir)/%.o: %.cpp | $(debug_dir)
	$(CPP) -c $< -o $@ $(build_flags) -MMD -MF $(@:.o=.d)
$(debug_dir)/%.d: # prevent catchall target from catching .d files

# Clean target
clean:
	rm -rf $(exe_release) $(exe_debug) build

# Create build directories if they don't exist
$(release_dir) $(debug_dir):
	mkdir -p $@
	

endif
# -----------------------------------------------------------------------------


# help target
help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "Available targets: "
	@echo "  release - Optimized release build."
	@echo "  debug   - debug build full of debug information, rutime asserts, and error detection. (very slow)"
	@echo "  clean   - Remove build files and executables."
	@echo "  help    - Display this help message."

# Catchall target
%:
	@echo "Unknown target '$@'. Use 'make help' to list available targets"
