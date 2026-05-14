# DOSCurl - HTTP Client for 16-bit DOS

A lightweight HTTP client for 16-bit PC-DOS environments, inspired by curl.


## Executable binary

[doscurl.exe](https://raw.githubusercontent.com/dotnsf/doscurl/refs/heads/main/cpp/doscurl.exe)


## Features

- HTTP GET and POST requests
- URL parsing
- Custom headers support
- Basic authentication support
- Verbose output mode
- File output support
- Configurable timeouts (connection and total operation)
- Works with packet drivers (NE2000, etc.)

## Requirements

### Build Environment (Windows Host)
- Windows 11 (or compatible)
- Open Watcom V2 installed at `C:\WATCOM`
- WATTCP library installed and configured

### Runtime Environment (DOS)
- 16-bit PC-DOS or compatible
- Packet driver (e.g., NE2000)
- TCP/IP stack (mTCP or similar)
- Network connectivity

## Building

### On Windows Host

1. Ensure Open Watcom is installed and `WATCOM` environment variable is set:
   ```batch
   set WATCOM=C:\WATCOM
   ```

2. Run the build script:
   ```batch
   build.bat
   ```

3. The executable will be created at `build\doscurl.exe`

### Using Makefile directly

```batch
set WATCOM=C:\WATCOM
set PATH=%WATCOM%\binw;%PATH%
wmake
```

### Cleaning build artifacts

```batch
clean.bat
```

Or:
```batch
wmake clean
```

## Usage

### Basic GET request
```
doscurl http://example.com/
```

### POST request with data
```
doscurl -X POST -d "key=value&name=test" http://example.com/api
```

### Verbose output
```
doscurl -v http://example.com/
```

### Save output to file
```
doscurl -o output.txt http://example.com/data.json
```

### Custom headers
```
doscurl -H "Authorization: Bearer token123" http://api.example.com/
```

### Basic authentication
```
doscurl -u "username:password" http://example.com/protected
```

### Custom timeouts
```
doscurl --connect-timeout 5 -m 60 http://example.com/
```

### Combined options
```
doscurl -v -X POST -d "data=test" -o result.txt http://example.com/api
```

## Command Line Options

| Option | Description |
|--------|-------------|
| `-X METHOD`, `--request METHOD` | HTTP method (GET, POST, HEAD, etc.) |
| `-d DATA`, `--data DATA` | POST data (automatically sets method to POST) |
| `-H HEADER`, `--header HEADER` | Add custom header (can be used multiple times, max 10) |
| `-u USER:PASS`, `--user USER:PASS` | Basic authentication credentials |
| `-v`, `--verbose` | Verbose output (shows connection details) |
| `-o FILE`, `--output FILE` | Write output to file instead of stdout |
| `-m SECONDS`, `--max-time SECONDS` | Maximum time for the entire operation (default: 30) |
| `--connect-timeout SECONDS` | Maximum time for connection (default: 10) |
| `--version` | Show version number |

## Testing in DOSBox-X

1. Configure DOSBox-X with NE2000 emulation
2. Load packet driver in DOS
3. Configure mTCP with IP address and DNS
4. Copy `doscurl.exe` to DOSBox-X environment
5. Run doscurl commands

Example DOSBox-X setup:
```
# Load NE2000 packet driver
ne2000

# Configure mTCP
dhcp

# Test doscurl
doscurl http://example.com/
```

## Project Structure

```
doscurl/
├── src/              # Source code
│   ├── main.c        # Main entry point
│   ├── utils.c       # Utility functions
│   ├── url.c         # URL parser
│   ├── socket.c      # Socket operations
│   └── http.c        # HTTP protocol handler
├── include/          # Header files
│   ├── doscurl.h     # Common definitions
│   ├── utils.h       # Utility functions
│   ├── url.h         # URL parser
│   ├── socket.h      # Socket operations
│   └── http.h        # HTTP protocol
├── build/            # Build output
├── test/             # Test scripts
├── docs/             # Documentation
├── Makefile          # Open Watcom makefile
├── build.bat         # Build script
├── clean.bat         # Clean script
└── README.md         # This file
```

## Limitations

- HTTP only (no HTTPS support)
- Limited to large memory model (64KB per segment)
- No keep-alive connections
- Basic HTTP/1.0 support
- No cookie management
- No redirect following
- No chunked transfer encoding

## Development Status

Current status: **Phase 5 Complete** - Timeout customization implemented

### Completed Phases
- ✅ Phase 1: Project structure and build system
- ✅ Phase 2: Basic network connectivity (TCP connection test)
- ✅ Phase 3: DOSBox-X testing and verification
- ✅ Phase 4: HTTP POST request implementation
- ✅ Phase 5: Timeout customization

### Upcoming Features (Phase 6)
- HTTP redirects (301, 302)
- Chunked transfer encoding
- Enhanced error handling

## License

This project is provided as-is for educational and personal use.

## Author

Developed for 16-bit DOS environments using Open Watcom C/C++ and WATTCP.