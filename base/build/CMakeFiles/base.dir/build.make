# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/bear/muduo-study/base

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/bear/muduo-study/base/build

# Include any dependencies generated for this target.
include CMakeFiles/base.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/base.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/base.dir/flags.make

CMakeFiles/base.dir/Date.cpp.o: CMakeFiles/base.dir/flags.make
CMakeFiles/base.dir/Date.cpp.o: ../Date.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bear/muduo-study/base/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/base.dir/Date.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/base.dir/Date.cpp.o -c /home/bear/muduo-study/base/Date.cpp

CMakeFiles/base.dir/Date.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/base.dir/Date.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bear/muduo-study/base/Date.cpp > CMakeFiles/base.dir/Date.cpp.i

CMakeFiles/base.dir/Date.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/base.dir/Date.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bear/muduo-study/base/Date.cpp -o CMakeFiles/base.dir/Date.cpp.s

CMakeFiles/base.dir/TimeStamp.cpp.o: CMakeFiles/base.dir/flags.make
CMakeFiles/base.dir/TimeStamp.cpp.o: ../TimeStamp.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bear/muduo-study/base/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/base.dir/TimeStamp.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/base.dir/TimeStamp.cpp.o -c /home/bear/muduo-study/base/TimeStamp.cpp

CMakeFiles/base.dir/TimeStamp.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/base.dir/TimeStamp.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bear/muduo-study/base/TimeStamp.cpp > CMakeFiles/base.dir/TimeStamp.cpp.i

CMakeFiles/base.dir/TimeStamp.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/base.dir/TimeStamp.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bear/muduo-study/base/TimeStamp.cpp -o CMakeFiles/base.dir/TimeStamp.cpp.s

# Object files for target base
base_OBJECTS = \
"CMakeFiles/base.dir/Date.cpp.o" \
"CMakeFiles/base.dir/TimeStamp.cpp.o"

# External object files for target base
base_EXTERNAL_OBJECTS =

libbase.a: CMakeFiles/base.dir/Date.cpp.o
libbase.a: CMakeFiles/base.dir/TimeStamp.cpp.o
libbase.a: CMakeFiles/base.dir/build.make
libbase.a: CMakeFiles/base.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/bear/muduo-study/base/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX static library libbase.a"
	$(CMAKE_COMMAND) -P CMakeFiles/base.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/base.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/base.dir/build: libbase.a

.PHONY : CMakeFiles/base.dir/build

CMakeFiles/base.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/base.dir/cmake_clean.cmake
.PHONY : CMakeFiles/base.dir/clean

CMakeFiles/base.dir/depend:
	cd /home/bear/muduo-study/base/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/bear/muduo-study/base /home/bear/muduo-study/base /home/bear/muduo-study/base/build /home/bear/muduo-study/base/build /home/bear/muduo-study/base/build/CMakeFiles/base.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/base.dir/depend

