# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study

# Include any dependencies generated for this target.
include simpleCache/CMakeFiles/simpleCache.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include simpleCache/CMakeFiles/simpleCache.dir/compiler_depend.make

# Include the progress variables for this target.
include simpleCache/CMakeFiles/simpleCache.dir/progress.make

# Include the compile flags for this target's objects.
include simpleCache/CMakeFiles/simpleCache.dir/flags.make

simpleCache/CMakeFiles/simpleCache.dir/cache.c.o: simpleCache/CMakeFiles/simpleCache.dir/flags.make
simpleCache/CMakeFiles/simpleCache.dir/cache.c.o: simpleCache/cache.c
simpleCache/CMakeFiles/simpleCache.dir/cache.c.o: simpleCache/CMakeFiles/simpleCache.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object simpleCache/CMakeFiles/simpleCache.dir/cache.c.o"
	cd /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study/simpleCache && /usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT simpleCache/CMakeFiles/simpleCache.dir/cache.c.o -MF CMakeFiles/simpleCache.dir/cache.c.o.d -o CMakeFiles/simpleCache.dir/cache.c.o -c /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study/simpleCache/cache.c

simpleCache/CMakeFiles/simpleCache.dir/cache.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/simpleCache.dir/cache.c.i"
	cd /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study/simpleCache && /usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study/simpleCache/cache.c > CMakeFiles/simpleCache.dir/cache.c.i

simpleCache/CMakeFiles/simpleCache.dir/cache.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/simpleCache.dir/cache.c.s"
	cd /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study/simpleCache && /usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study/simpleCache/cache.c -o CMakeFiles/simpleCache.dir/cache.c.s

simpleCache/CMakeFiles/simpleCache.dir/stree.c.o: simpleCache/CMakeFiles/simpleCache.dir/flags.make
simpleCache/CMakeFiles/simpleCache.dir/stree.c.o: simpleCache/stree.c
simpleCache/CMakeFiles/simpleCache.dir/stree.c.o: simpleCache/CMakeFiles/simpleCache.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object simpleCache/CMakeFiles/simpleCache.dir/stree.c.o"
	cd /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study/simpleCache && /usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT simpleCache/CMakeFiles/simpleCache.dir/stree.c.o -MF CMakeFiles/simpleCache.dir/stree.c.o.d -o CMakeFiles/simpleCache.dir/stree.c.o -c /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study/simpleCache/stree.c

simpleCache/CMakeFiles/simpleCache.dir/stree.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/simpleCache.dir/stree.c.i"
	cd /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study/simpleCache && /usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study/simpleCache/stree.c > CMakeFiles/simpleCache.dir/stree.c.i

simpleCache/CMakeFiles/simpleCache.dir/stree.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/simpleCache.dir/stree.c.s"
	cd /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study/simpleCache && /usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study/simpleCache/stree.c -o CMakeFiles/simpleCache.dir/stree.c.s

# Object files for target simpleCache
simpleCache_OBJECTS = \
"CMakeFiles/simpleCache.dir/cache.c.o" \
"CMakeFiles/simpleCache.dir/stree.c.o"

# External object files for target simpleCache
simpleCache_EXTERNAL_OBJECTS =

simpleCache/libsimpleCache.so: simpleCache/CMakeFiles/simpleCache.dir/cache.c.o
simpleCache/libsimpleCache.so: simpleCache/CMakeFiles/simpleCache.dir/stree.c.o
simpleCache/libsimpleCache.so: simpleCache/CMakeFiles/simpleCache.dir/build.make
simpleCache/libsimpleCache.so: simpleCache/CMakeFiles/simpleCache.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C shared library libsimpleCache.so"
	cd /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study/simpleCache && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/simpleCache.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
simpleCache/CMakeFiles/simpleCache.dir/build: simpleCache/libsimpleCache.so
.PHONY : simpleCache/CMakeFiles/simpleCache.dir/build

simpleCache/CMakeFiles/simpleCache.dir/clean:
	cd /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study/simpleCache && $(CMAKE_COMMAND) -P CMakeFiles/simpleCache.dir/cmake_clean.cmake
.PHONY : simpleCache/CMakeFiles/simpleCache.dir/clean

simpleCache/CMakeFiles/simpleCache.dir/depend:
	cd /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study/simpleCache /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study/simpleCache /afs/andrew.cmu.edu/usr3/dbhegde/private/15618/project/coherence_protocol_study/simpleCache/CMakeFiles/simpleCache.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : simpleCache/CMakeFiles/simpleCache.dir/depend

