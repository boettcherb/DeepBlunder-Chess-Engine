# Usage:
#   "make" or "make release" for the optimized release build
#     Creates executable: deepblunder
#   "make debug" for the debug build
#     Creates executable: deepblunder_debug
#   "make clean" to remove executables and build directories

.DEFAULT_GOAL := release
.PHONY: release debug clean

# Source files
cpp_files = Attack.cpp Board.cpp BoardString.cpp Defs.cpp Engine.cpp Evaluate.cpp HashKey.cpp Main.cpp MoveList.cpp Perft.cpp Table.cpp

# Build directories
release_dir = build/release
debug_dir = build/debug

# Create names for object and dependency files by prepending the correct path (either
# build/release or build/debug) and appending the correct extension (either .o or .d)
release_obj_files = $(patsubst %.cpp, $(release_dir)/%.o, $(cpp_files))
release_dep_files = $(patsubst %.cpp, $(release_dir)/%.d, $(cpp_files))
debug_obj_files = $(patsubst %.cpp, $(debug_dir)/%.o, $(cpp_files))
debug_dep_files = $(patsubst %.cpp, $(debug_dir)/%.d, $(cpp_files))

# Build flags
build_flags = -std=c++17 -Wall -Wextra -Werror -march=native

# Release target
release: build_flags += -O3 -DNDEBUG -s
release: deepblunder
deepblunder: $(release_obj_files)
	g++ $(release_obj_files) -o deepblunder $(build_flags) -flto
-include $(release_dep_files)

# Debug target
debug: build_flags += -g -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=address -fsanitize=undefined -fno-sanitize-recover
debug: deepblunder_debug
deepblunder_debug: $(debug_obj_files)
	g++ $(debug_obj_files) -o deepblunder_debug $(build_flags)
-include $(debug_dep_files)

# Compile source files into object files and generate dependencies
$(release_dir)/%.o: %.cpp | $(release_dir)
	g++ -c $< -o $@ $(build_flags) -MMD -MF $(@:.o=.d)
$(debug_dir)/%.o: %.cpp | $(debug_dir)
	g++ -c $< -o $@ $(build_flags) -MMD -MF $(@:.o=.d)

# Create build directories if they don't exist
$(release_dir) $(debug_dir):
	mkdir -p $@

# Clean target
clean:
	rm -rf deepblunder deepblunder_debug $(release_dir) $(debug_dir)
