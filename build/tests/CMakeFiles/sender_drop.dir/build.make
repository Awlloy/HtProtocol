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
CMAKE_SOURCE_DIR = /home/hasee/myProject/科协通信协议/科协通信协议二代/HtProtocol

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/hasee/myProject/科协通信协议/科协通信协议二代/HtProtocol/build

# Include any dependencies generated for this target.
include tests/CMakeFiles/sender_drop.dir/depend.make

# Include the progress variables for this target.
include tests/CMakeFiles/sender_drop.dir/progress.make

# Include the compile flags for this target's objects.
include tests/CMakeFiles/sender_drop.dir/flags.make

tests/CMakeFiles/sender_drop.dir/sender_drop.cpp.o: tests/CMakeFiles/sender_drop.dir/flags.make
tests/CMakeFiles/sender_drop.dir/sender_drop.cpp.o: ../tests/sender_drop.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hasee/myProject/科协通信协议/科协通信协议二代/HtProtocol/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object tests/CMakeFiles/sender_drop.dir/sender_drop.cpp.o"
	cd /home/hasee/myProject/科协通信协议/科协通信协议二代/HtProtocol/build/tests && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/sender_drop.dir/sender_drop.cpp.o -c /home/hasee/myProject/科协通信协议/科协通信协议二代/HtProtocol/tests/sender_drop.cpp

tests/CMakeFiles/sender_drop.dir/sender_drop.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/sender_drop.dir/sender_drop.cpp.i"
	cd /home/hasee/myProject/科协通信协议/科协通信协议二代/HtProtocol/build/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/hasee/myProject/科协通信协议/科协通信协议二代/HtProtocol/tests/sender_drop.cpp > CMakeFiles/sender_drop.dir/sender_drop.cpp.i

tests/CMakeFiles/sender_drop.dir/sender_drop.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/sender_drop.dir/sender_drop.cpp.s"
	cd /home/hasee/myProject/科协通信协议/科协通信协议二代/HtProtocol/build/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/hasee/myProject/科协通信协议/科协通信协议二代/HtProtocol/tests/sender_drop.cpp -o CMakeFiles/sender_drop.dir/sender_drop.cpp.s

# Object files for target sender_drop
sender_drop_OBJECTS = \
"CMakeFiles/sender_drop.dir/sender_drop.cpp.o"

# External object files for target sender_drop
sender_drop_EXTERNAL_OBJECTS =

tests/sender_drop: tests/CMakeFiles/sender_drop.dir/sender_drop.cpp.o
tests/sender_drop: tests/CMakeFiles/sender_drop.dir/build.make
tests/sender_drop: libHtProtocol_shared.so
tests/sender_drop: tests/CMakeFiles/sender_drop.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/hasee/myProject/科协通信协议/科协通信协议二代/HtProtocol/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable sender_drop"
	cd /home/hasee/myProject/科协通信协议/科协通信协议二代/HtProtocol/build/tests && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/sender_drop.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tests/CMakeFiles/sender_drop.dir/build: tests/sender_drop

.PHONY : tests/CMakeFiles/sender_drop.dir/build

tests/CMakeFiles/sender_drop.dir/clean:
	cd /home/hasee/myProject/科协通信协议/科协通信协议二代/HtProtocol/build/tests && $(CMAKE_COMMAND) -P CMakeFiles/sender_drop.dir/cmake_clean.cmake
.PHONY : tests/CMakeFiles/sender_drop.dir/clean

tests/CMakeFiles/sender_drop.dir/depend:
	cd /home/hasee/myProject/科协通信协议/科协通信协议二代/HtProtocol/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/hasee/myProject/科协通信协议/科协通信协议二代/HtProtocol /home/hasee/myProject/科协通信协议/科协通信协议二代/HtProtocol/tests /home/hasee/myProject/科协通信协议/科协通信协议二代/HtProtocol/build /home/hasee/myProject/科协通信协议/科协通信协议二代/HtProtocol/build/tests /home/hasee/myProject/科协通信协议/科协通信协议二代/HtProtocol/build/tests/CMakeFiles/sender_drop.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tests/CMakeFiles/sender_drop.dir/depend

