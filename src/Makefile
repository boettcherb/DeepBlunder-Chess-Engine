# run "make help" for usage details

.DEFAULT_GOAL := help
.PHONY: help release debug clean

build_flags = -std=c++20 -Wall -Wextra -Werror -march=native
cpp_files = attack.cpp board.cpp boardstring.cpp defs.cpp engine.cpp evaluate.cpp hashkey.cpp main.cpp movelist.cpp perft.cpp table.cpp
CPP = g++

# Build directories
release_dir = build/release
debug_dir = build/debug

# Create names for object and dependency files by prepending the correct path (either
# build/release or build/debug) and appending the correct extension (either .o or .d)
release_obj_files = $(patsubst %.cpp, $(release_dir)/%.o, $(cpp_files))
release_dep_files = $(patsubst %.cpp, $(release_dir)/%.d, $(cpp_files))
debug_obj_files = $(patsubst %.cpp, $(debug_dir)/%.o, $(cpp_files))
debug_dep_files = $(patsubst %.cpp, $(debug_dir)/%.d, $(cpp_files))

# Release target
release: build_flags += -O3 -DNDEBUG -s
release: deepblunder
deepblunder: $(release_obj_files)
	$(CPP) $(release_obj_files) -o deepblunder $(build_flags) -flto
-include $(release_dep_files)
$(release_dir)/%.o: %.cpp | $(release_dir)
	$(CPP) -c $< -o $@ $(build_flags) -MMD -MF $(@:.o=.d)

# Debug target
debug: build_flags += -g -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=address -fsanitize=undefined -fno-sanitize-recover
debug: deepblunder_debug
deepblunder_debug: $(debug_obj_files)
	$(CPP) $(debug_obj_files) -o deepblunder_debug $(build_flags)
-include $(debug_dep_files)
$(debug_dir)/%.o: %.cpp | $(debug_dir)
	$(CPP) -c $< -o $@ $(build_flags) -MMD -MF $(@:.o=.d)

# Clean target
clean:
	rm -rf deepblunder deepblunder_debug $(release_dir) $(debug_dir)

# help target
help:
	@echo "Available targets: "
	@echo "  release - Optimized release build."
	@echo "  debug   - debug build full of debug information, rutime asserts, and error detection. (very slow)"
	@echo "  clean   - Remove build files / executables."
	@echo "  help    - Display this help message."

# Create build directories if they don't exist
$(release_dir) $(debug_dir):
	mkdir -p $@
