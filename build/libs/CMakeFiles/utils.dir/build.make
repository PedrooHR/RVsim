# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.18

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Disable VCS-based implicit rules.
% : %,v


# Disable VCS-based implicit rules.
% : RCS/%


# Disable VCS-based implicit rules.
% : RCS/%,v


# Disable VCS-based implicit rules.
% : SCCS/s.%


# Disable VCS-based implicit rules.
% : s.%


.SUFFIXES: .hpux_make_needs_suffix_list


# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/pedro/Documentos/ARQ2/RVsim

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/pedro/Documentos/ARQ2/RVsim/build

# Include any dependencies generated for this target.
include libs/CMakeFiles/utils.dir/depend.make

# Include the progress variables for this target.
include libs/CMakeFiles/utils.dir/progress.make

# Include the compile flags for this target's objects.
include libs/CMakeFiles/utils.dir/flags.make

# Object files for target utils
utils_OBJECTS =

# External object files for target utils
utils_EXTERNAL_OBJECTS =

libs/libutils.a: libs/CMakeFiles/utils.dir/build.make
libs/libutils.a: libs/CMakeFiles/utils.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/pedro/Documentos/ARQ2/RVsim/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Linking CXX static library libutils.a"
	cd /home/pedro/Documentos/ARQ2/RVsim/build/libs && $(CMAKE_COMMAND) -P CMakeFiles/utils.dir/cmake_clean_target.cmake
	cd /home/pedro/Documentos/ARQ2/RVsim/build/libs && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/utils.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
libs/CMakeFiles/utils.dir/build: libs/libutils.a

.PHONY : libs/CMakeFiles/utils.dir/build

libs/CMakeFiles/utils.dir/clean:
	cd /home/pedro/Documentos/ARQ2/RVsim/build/libs && $(CMAKE_COMMAND) -P CMakeFiles/utils.dir/cmake_clean.cmake
.PHONY : libs/CMakeFiles/utils.dir/clean

libs/CMakeFiles/utils.dir/depend:
	cd /home/pedro/Documentos/ARQ2/RVsim/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/pedro/Documentos/ARQ2/RVsim /home/pedro/Documentos/ARQ2/RVsim/libs /home/pedro/Documentos/ARQ2/RVsim/build /home/pedro/Documentos/ARQ2/RVsim/build/libs /home/pedro/Documentos/ARQ2/RVsim/build/libs/CMakeFiles/utils.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : libs/CMakeFiles/utils.dir/depend

