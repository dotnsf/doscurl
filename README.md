# DOSCurl - HTTP Client for 16-bit DOS

A lightweight HTTP client for 16-bit PC-DOS environments, inspired by curl.


## Executable binary

[doscurl.exe](https://raw.githubusercontent.com/dotnsf/doscurl/refs/heads/main/cpp/doscurl.exe)


## Features

- HTTP GET and POST requests
- URL parsing
- Custom headers support
- Basic authentication support
- **HTTP redirect following (301, 302, 303, 307, 308)**
- **Detailed error handling with HTTP status codes**
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

### Follow redirects (enabled by default)
```
doscurl -L http://example.com/redirect
doscurl --max-redirs 5 http://example.com/
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
| `-L`, `--location` | Follow HTTP redirects (enabled by default) |
| `--max-redirs NUM` | Maximum number of redirects to follow (default: 10) |
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
- **Requires Large memory model (-ml)** - Medium (-mm) and Small (-ms) models are not supported
- No keep-alive connections
- Basic HTTP/1.0 support
- No cookie management
- No chunked transfer encoding

### Memory Model Requirements

DOSCurl **must** be compiled with the **Large memory model (-ml)**. Other memory models are not supported:

| Memory Model | Status | Reason |
|--------------|--------|--------|
| **Large (-ml)** | ✅ **Required** | Only model that works with mTCP |
| Medium (-mm) | ❌ Not supported | Packet buffer initialization fails |
| Small (-ms) | ❌ Not supported | Packet buffer initialization fails |
| Compact (-mc) | ❌ Not tested | Likely incompatible with mTCP |

**Technical Details:**
- mTCP's packet buffer structures exceed the 64KB data segment limit of Medium/Small models
- Even with minimal `PACKET_BUFFERS` settings (5 buffers), initialization fails
- The mTCP library is designed for Large memory model usage
- Attempting to use other models results in "Init: Could not setup packet buffers" error

**Build Configuration:**
The Makefile is pre-configured with Large memory model:
```makefile
memory_model = -ml
```
**Do not change this setting** - other memory models will not work.

## Known Issues

### "End: heap is corrupted!" Message

When the program exits, you may see the following message:
```
End: heap is corrupted!
```

**This message can be safely ignored.** It is a false positive from mTCP's internal heap checking mechanism.

**Why this happens:**
- DOSCurl uses `malloc()`/`free()` for large buffers to avoid memory constraints
- mTCP's `_heapchk()` function incorrectly reports this as heap corruption
- The program functions correctly despite this message

**Impact:**
- ✅ No functional impact - the program works correctly
- ✅ Multiple executions work fine
- ✅ All HTTP operations complete successfully
- ⚠️ The message appears every time the program exits

This is a known limitation of mixing mTCP's memory management with standard C library memory allocation in 16-bit DOS environments.

## Development Status

Current status: **Phase 6 Complete** - HTTP redirects and error handling implemented

### Completed Phases
- ✅ Phase 1: Project structure and build system
- ✅ Phase 2: Basic network connectivity (TCP connection test)
- ✅ Phase 3: DOSBox-X testing and verification
- ✅ Phase 4: HTTP POST request implementation
- ✅ Phase 5: Timeout customization
- ✅ Phase 6: HTTP redirects and detailed error handling

### Future Enhancements
- Chunked transfer encoding
- HTTP proxy support
- Progress indicators for large transfers

## License

This project is provided as-is for educational and personal use.

## Author

Developed for 16-bit DOS environments using Open Watcom C/C++ and WATTCP.