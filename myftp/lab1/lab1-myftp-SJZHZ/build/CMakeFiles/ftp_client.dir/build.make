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
CMAKE_SOURCE_DIR = /home/zy/NetworkTest/lab1-myftp-SJZHZ

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/zy/NetworkTest/lab1-myftp-SJZHZ/build

# Include any dependencies generated for this target.
include CMakeFiles/ftp_client.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/ftp_client.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/ftp_client.dir/flags.make

CMakeFiles/ftp_client.dir/ftp_client.c.o: CMakeFiles/ftp_client.dir/flags.make
CMakeFiles/ftp_client.dir/ftp_client.c.o: ../ftp_client.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zy/NetworkTest/lab1-myftp-SJZHZ/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/ftp_client.dir/ftp_client.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/ftp_client.dir/ftp_client.c.o   -c /home/zy/NetworkTest/lab1-myftp-SJZHZ/ftp_client.c

CMakeFiles/ftp_client.dir/ftp_client.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/ftp_client.dir/ftp_client.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/zy/NetworkTest/lab1-myftp-SJZHZ/ftp_client.c > CMakeFiles/ftp_client.dir/ftp_client.c.i

CMakeFiles/ftp_client.dir/ftp_client.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/ftp_client.dir/ftp_client.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/zy/NetworkTest/lab1-myftp-SJZHZ/ftp_client.c -o CMakeFiles/ftp_client.dir/ftp_client.c.s

# Object files for target ftp_client
ftp_client_OBJECTS = \
"CMakeFiles/ftp_client.dir/ftp_client.c.o"

# External object files for target ftp_client
ftp_client_EXTERNAL_OBJECTS =

ftp_client: CMakeFiles/ftp_client.dir/ftp_client.c.o
ftp_client: CMakeFiles/ftp_client.dir/build.make
ftp_client: CMakeFiles/ftp_client.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/zy/NetworkTest/lab1-myftp-SJZHZ/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable ftp_client"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/ftp_client.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/ftp_client.dir/build: ftp_client

.PHONY : CMakeFiles/ftp_client.dir/build

CMakeFiles/ftp_client.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/ftp_client.dir/cmake_clean.cmake
.PHONY : CMakeFiles/ftp_client.dir/clean

CMakeFiles/ftp_client.dir/depend:
	cd /home/zy/NetworkTest/lab1-myftp-SJZHZ/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/zy/NetworkTest/lab1-myftp-SJZHZ /home/zy/NetworkTest/lab1-myftp-SJZHZ /home/zy/NetworkTest/lab1-myftp-SJZHZ/build /home/zy/NetworkTest/lab1-myftp-SJZHZ/build /home/zy/NetworkTest/lab1-myftp-SJZHZ/build/CMakeFiles/ftp_client.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/ftp_client.dir/depend

