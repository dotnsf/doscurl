# DOSCurl - HTTP Client for 16-bit DOS

A lightweight HTTP client for 16-bit PC-DOS environments, inspired by curl.


## Executable binary

[doscurl.exe](https://raw.githubusercontent.com/dotnsf/doscurl/refs/heads/main/cpp/doscurl.exe)


## Features

- HTTP GET and POST requests
- URL parsing
- Custom headers support
- Verbose output mode
- File output support
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

### Combined options
```
doscurl -v -X POST -d "data=test" -o result.txt http://example.com/api
```

## Command Line Options

| Option | Description |
|--------|-------------|
| `-X METHOD` | HTTP method (GET, POST, HEAD) |
| `-d DATA` | POST data (automatically sets method to POST) |
| `-H HEADER` | Add custom header (can be used multiple times, max 10) |
| `-v` | Verbose output (shows connection details) |
| `-o FILE` | Write output to file instead of stdout |
| `--help` | Show help message |

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
- Limited to small memory model (64KB code + 64KB data)
- No keep-alive connections
- Basic HTTP/1.0 and partial HTTP/1.1 support
- No cookie management
- No authentication (except via custom headers)

## Development Status

See [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md) for detailed development roadmap.

Current status: Phase 1 - Project structure and build system complete

## License

This project is provided as-is for educational and personal use.

## Author

Developed for 16-bit DOS environments using Open Watcom C/C++ and WATTCP.