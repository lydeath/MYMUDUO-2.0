# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

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
CMAKE_SOURCE_DIR = /home/luoyan/桌面/muduo2.0

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/luoyan/桌面/muduo2.0/build

# Include any dependencies generated for this target.
include src/memory/test/CMakeFiles/MemoryPoolTest.dir/depend.make

# Include the progress variables for this target.
include src/memory/test/CMakeFiles/MemoryPoolTest.dir/progress.make

# Include the compile flags for this target's objects.
include src/memory/test/CMakeFiles/MemoryPoolTest.dir/flags.make

src/memory/test/CMakeFiles/MemoryPoolTest.dir/MemoryPoolTest.cc.o: src/memory/test/CMakeFiles/MemoryPoolTest.dir/flags.make
src/memory/test/CMakeFiles/MemoryPoolTest.dir/MemoryPoolTest.cc.o: ../src/memory/test/MemoryPoolTest.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/luoyan/桌面/muduo2.0/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/memory/test/CMakeFiles/MemoryPoolTest.dir/MemoryPoolTest.cc.o"
	cd /home/luoyan/桌面/muduo2.0/build/src/memory/test && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/MemoryPoolTest.dir/MemoryPoolTest.cc.o -c /home/luoyan/桌面/muduo2.0/src/memory/test/MemoryPoolTest.cc

src/memory/test/CMakeFiles/MemoryPoolTest.dir/MemoryPoolTest.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/MemoryPoolTest.dir/MemoryPoolTest.cc.i"
	cd /home/luoyan/桌面/muduo2.0/build/src/memory/test && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/luoyan/桌面/muduo2.0/src/memory/test/MemoryPoolTest.cc > CMakeFiles/MemoryPoolTest.dir/MemoryPoolTest.cc.i

src/memory/test/CMakeFiles/MemoryPoolTest.dir/MemoryPoolTest.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/MemoryPoolTest.dir/MemoryPoolTest.cc.s"
	cd /home/luoyan/桌面/muduo2.0/build/src/memory/test && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/luoyan/桌面/muduo2.0/src/memory/test/MemoryPoolTest.cc -o CMakeFiles/MemoryPoolTest.dir/MemoryPoolTest.cc.s

src/memory/test/CMakeFiles/MemoryPoolTest.dir/MemoryPoolTest.cc.o.requires:

.PHONY : src/memory/test/CMakeFiles/MemoryPoolTest.dir/MemoryPoolTest.cc.o.requires

src/memory/test/CMakeFiles/MemoryPoolTest.dir/MemoryPoolTest.cc.o.provides: src/memory/test/CMakeFiles/MemoryPoolTest.dir/MemoryPoolTest.cc.o.requires
	$(MAKE) -f src/memory/test/CMakeFiles/MemoryPoolTest.dir/build.make src/memory/test/CMakeFiles/MemoryPoolTest.dir/MemoryPoolTest.cc.o.provides.build
.PHONY : src/memory/test/CMakeFiles/MemoryPoolTest.dir/MemoryPoolTest.cc.o.provides

src/memory/test/CMakeFiles/MemoryPoolTest.dir/MemoryPoolTest.cc.o.provides.build: src/memory/test/CMakeFiles/MemoryPoolTest.dir/MemoryPoolTest.cc.o


# Object files for target MemoryPoolTest
MemoryPoolTest_OBJECTS = \
"CMakeFiles/MemoryPoolTest.dir/MemoryPoolTest.cc.o"

# External object files for target MemoryPoolTest
MemoryPoolTest_EXTERNAL_OBJECTS =

../src/memory/test/MemoryPoolTest: src/memory/test/CMakeFiles/MemoryPoolTest.dir/MemoryPoolTest.cc.o
../src/memory/test/MemoryPoolTest: src/memory/test/CMakeFiles/MemoryPoolTest.dir/build.make
../src/memory/test/MemoryPoolTest: ../lib/libtiny_network.so
../src/memory/test/MemoryPoolTest: src/memory/test/CMakeFiles/MemoryPoolTest.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/luoyan/桌面/muduo2.0/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../../../../src/memory/test/MemoryPoolTest"
	cd /home/luoyan/桌面/muduo2.0/build/src/memory/test && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/MemoryPoolTest.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/memory/test/CMakeFiles/MemoryPoolTest.dir/build: ../src/memory/test/MemoryPoolTest

.PHONY : src/memory/test/CMakeFiles/MemoryPoolTest.dir/build

src/memory/test/CMakeFiles/MemoryPoolTest.dir/requires: src/memory/test/CMakeFiles/MemoryPoolTest.dir/MemoryPoolTest.cc.o.requires

.PHONY : src/memory/test/CMakeFiles/MemoryPoolTest.dir/requires

src/memory/test/CMakeFiles/MemoryPoolTest.dir/clean:
	cd /home/luoyan/桌面/muduo2.0/build/src/memory/test && $(CMAKE_COMMAND) -P CMakeFiles/MemoryPoolTest.dir/cmake_clean.cmake
.PHONY : src/memory/test/CMakeFiles/MemoryPoolTest.dir/clean

src/memory/test/CMakeFiles/MemoryPoolTest.dir/depend:
	cd /home/luoyan/桌面/muduo2.0/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/luoyan/桌面/muduo2.0 /home/luoyan/桌面/muduo2.0/src/memory/test /home/luoyan/桌面/muduo2.0/build /home/luoyan/桌面/muduo2.0/build/src/memory/test /home/luoyan/桌面/muduo2.0/build/src/memory/test/CMakeFiles/MemoryPoolTest.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/memory/test/CMakeFiles/MemoryPoolTest.dir/depend

