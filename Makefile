#
# DOSCurl Makefile for Open Watcom C/C++
#

# Compiler and tools
CC = wcl
CFLAGS = -ms -0 -w4 -zq -bt=dos
INCLUDES = -I.\include -IC:\WATTCP\inc -IC:\WATCOM\h
WATTCP_LIB = C:\WATCOM\lib286\wattcpws.lib

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
LDFLAGS = -l=dos

# Source files
SOURCES = $(SRC_DIR)\main.c $(SRC_DIR)\utils.c $(SRC_DIR)\url.c $(SRC_DIR)\socket.c $(SRC_DIR)\http.c

# Object files
OBJECTS = $(BUILD_DIR)\main.obj $(BUILD_DIR)\utils.obj $(BUILD_DIR)\url.obj $(BUILD_DIR)\socket.obj $(BUILD_DIR)\http.obj

# Target executable
TARGET = $(BUILD_DIR)\doscurl.exe

# Default target
all: $(TARGET)

# Link executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -fe=$(TARGET) $(OBJECTS) $(WATTCP_LIB)

# Compile main.c
$(BUILD_DIR)\main.obj: $(SRC_DIR)\main.c $(INC_DIR)\doscurl.h $(INC_DIR)\utils.h $(INC_DIR)\url.h $(INC_DIR)\socket.h $(INC_DIR)\http.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -fo=$(BUILD_DIR)\main.obj $(SRC_DIR)\main.c

# Compile utils.c
$(BUILD_DIR)\utils.obj: $(SRC_DIR)\utils.c $(INC_DIR)\utils.h $(INC_DIR)\doscurl.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -fo=$(BUILD_DIR)\utils.obj $(SRC_DIR)\utils.c

# Compile url.c
$(BUILD_DIR)\url.obj: $(SRC_DIR)\url.c $(INC_DIR)\url.h $(INC_DIR)\doscurl.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -fo=$(BUILD_DIR)\url.obj $(SRC_DIR)\url.c

# Compile socket.c
$(BUILD_DIR)\socket.obj: $(SRC_DIR)\socket.c $(INC_DIR)\socket.h $(INC_DIR)\doscurl.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -fo=$(BUILD_DIR)\socket.obj $(SRC_DIR)\socket.c

# Compile http.c
$(BUILD_DIR)\http.obj: $(SRC_DIR)\http.c $(INC_DIR)\http.h $(INC_DIR)\doscurl.h $(INC_DIR)\socket.h $(INC_DIR)\url.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -fo=$(BUILD_DIR)\http.obj $(SRC_DIR)\http.c

# Clean build artifacts
clean:
	-del $(BUILD_DIR)\*.obj
	-del $(BUILD_DIR)\*.exe
	-del $(BUILD_DIR)\*.err

# Help
help:
	@echo DOSCurl Makefile
	@echo.
	@echo Targets:
	@echo   all     - Build doscurl.exe (default)
	@echo   clean   - Remove build artifacts
	@echo   help    - Show this help message
	@echo.
	@echo Usage:
	@echo   wmake
	@echo   wmake clean

# Made with Bob
