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
include rvsim/CMakeFiles/rvsim.dir/depend.make

# Include the progress variables for this target.
include rvsim/CMakeFiles/rvsim.dir/progress.make

# Include the compile flags for this target's objects.
include rvsim/CMakeFiles/rvsim.dir/flags.make

# Object files for target rvsim
rvsim_OBJECTS =

# External object files for target rvsim
rvsim_EXTERNAL_OBJECTS =

rvsim/rvsim: rvsim/CMakeFiles/rvsim.dir/build.make
rvsim/rvsim: libs/libutils.a
rvsim/rvsim: libs/libmemory.a
rvsim/rvsim: libs/libisa.a
rvsim/rvsim: libs/libregisters.a
rvsim/rvsim: libs/libprocessor.a
rvsim/rvsim: libs/libutils.a
rvsim/rvsim: libs/libmemory.a
rvsim/rvsim: libs/libisa.a
rvsim/rvsim: libs/libregisters.a
rvsim/rvsim: rvsim/CMakeFiles/rvsim.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/pedro/Documentos/ARQ2/RVsim/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Linking CXX executable rvsim"
	cd /home/pedro/Documentos/ARQ2/RVsim/build/rvsim && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/rvsim.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
rvsim/CMakeFiles/rvsim.dir/build: rvsim/rvsim

.PHONY : rvsim/CMakeFiles/rvsim.dir/build

rvsim/CMakeFiles/rvsim.dir/clean:
	cd /home/pedro/Documentos/ARQ2/RVsim/build/rvsim && $(CMAKE_COMMAND) -P CMakeFiles/rvsim.dir/cmake_clean.cmake
.PHONY : rvsim/CMakeFiles/rvsim.dir/clean

rvsim/CMakeFiles/rvsim.dir/depend:
	cd /home/pedro/Documentos/ARQ2/RVsim/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/pedro/Documentos/ARQ2/RVsim /home/pedro/Documentos/ARQ2/RVsim/rvsim /home/pedro/Documentos/ARQ2/RVsim/build /home/pedro/Documentos/ARQ2/RVsim/build/rvsim /home/pedro/Documentos/ARQ2/RVsim/build/rvsim/CMakeFiles/rvsim.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : rvsim/CMakeFiles/rvsim.dir/depend

