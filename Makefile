#
# DOSCurl Makefile for Open Watcom C++ with mTCP
#

# Compiler and tools
CXX = wpp
CXXFLAGS = -ml -0 -w4 -zq -bt=dos -DCFG_H="doscurl.cfg"
INCLUDES = -IC:\mTCP\src\TCPINC -IC:\WATCOM\h
MTCP_LIB = C:\mTCP\TCPLIB\MTCPWL.LIB

# Directories
CPP_DIR = cpp
BUILD_DIR = build
LDFLAGS = -l=dos

# Source files
SOURCES = $(CPP_DIR)\doscurl.cpp

# Object files
OBJECTS = $(BUILD_DIR)\doscurl.obj

# Target executable
TARGET = $(BUILD_DIR)\doscurl.exe

# Default target
all: $(TARGET)

# Link executable
$(TARGET): $(OBJECTS)
	wlink system dos option quiet option map=$(BUILD_DIR)\doscurl.map file $(OBJECTS) library $(MTCP_LIB) name $(TARGET)

# Compile doscurl.cpp
$(BUILD_DIR)\doscurl.obj: $(SOURCES) $(CPP_DIR)\doscurl.cfg
	$(CXX) $(CXXFLAGS) $(INCLUDES) -fo=$(BUILD_DIR)\doscurl.obj $(SOURCES)

# Clean build artifacts
clean:
	-del $(BUILD_DIR)\*.obj
	-del $(BUILD_DIR)\*.exe
	-del $(BUILD_DIR)\*.err
	-del $(BUILD_DIR)\*.map

# Help
help:
	@echo DOSCurl Makefile (mTCP C++ version)
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
