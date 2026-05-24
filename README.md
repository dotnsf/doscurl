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
- **Three-level verbosity control** (quiet, verbose, extra verbose)
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

### Prerequisites

1. **Open Watcom V2** installed at `C:\WATCOM`
2. **mTCP source code** installed at `C:\mTCP\src`
3. **WATCOM environment variable** set to `C:\WATCOM`

### Build Steps

1. Navigate to the `cpp` directory:
   ```batch
   cd doscurl\cpp
   ```

2. Run the build using wmake:
   ```batch
   wmake all
   ```

3. The executable will be created as `doscurl.exe` in the `cpp` directory

### Alternative: Using build.bat

```batch
cd doscurl\cpp
build.bat
```

### Cleaning build artifacts

```batch
cd doscurl\cpp
wmake clean
```

This will remove all `.obj`, `.exe`, `.map`, and `.err` files from the build directory.

## Usage

### Basic GET request
```
doscurl http://example.com/
```

### POST request with data
```
doscurl -X POST -d "key=value&name=test" http://example.com/api
```

### Verbose output (show HTTP headers)
```
doscurl -v http://example.com/
```

### Extra verbose output (show debug information)
```
doscurl -vv http://example.com/
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
| `-v`, `--verbose` | Verbose output (shows HTTP headers) |
| `-vv` | Extra verbose output (shows debug information) |
| `-o FILE`, `--output FILE` | Write output to file instead of stdout |
| `-L`, `--location` | Follow HTTP redirects (enabled by default) |
| `--max-redirs NUM` | Maximum number of redirects to follow (default: 10) |
| `-m SECONDS`, `--max-time SECONDS` | Maximum time for the entire operation (default: 30) |
| `--connect-timeout SECONDS` | Maximum time for connection (default: 10) |
| `--version` | Show version number |

## Output Verbosity Levels

DOSCurl supports three levels of output verbosity:

### Default (no option)
- **Output**: HTTP body only
- **Use case**: When you only need the response content
```bash
doscurl http://example.com/
# Output: <body content only>
```

### Verbose mode (`-v`)
- **Output**: HTTP headers + blank line + HTTP body
- **Use case**: When you need to see response headers (status code, content type, etc.)
```bash
doscurl -v http://example.com/
# Output:
# HTTP/1.1 200 OK
# Content-Type: text/html
# Content-Length: 1234
# ...
#
# <body content>
```

### Extra verbose mode (`-vv`)
- **Output**: Debug information + HTTP headers + blank line + HTTP body
- **Use case**: For debugging connection issues or understanding the full request/response cycle
- **Includes**:
  - DNS resolution details
  - Connection establishment messages
  - HTTP request being sent
  - Request parameters (method, host, port, path)
  - Response reception progress
  - Redirect information
  - File save confirmations
  - Total bytes received

```bash
doscurl -vv http://example.com/
# Output:
# Method: GET
# Host: example.com
# Port: 80
# Path: /
#
# Resolving example.com... resolved to 93.184.216.34
# Connecting to example.com:80... connected
# Sending GET request...
# --- Request ---
# GET / HTTP/1.0
# Host: example.com
# User-Agent: DOSCurl/0.1.0
# Connection: close
#
# --- End Request ---
# Receiving response...
# HTTP/1.1 200 OK
# Content-Type: text/html
# ...
#
# <body content>
#
# Received 1234 bytes total
```

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
├── cpp/              # C++ implementation (current version)
│   ├── doscurl.cpp   # Main source file
│   ├── doscurl.cfg   # mTCP configuration
│   ├── Makefile      # Open Watcom makefile
│   ├── build.bat     # Build script
│   ├── README.md     # Detailed documentation
│   └── TESTING.md    # Testing guide
├── src/              # Legacy C implementation (deprecated)
├── include/          # Legacy headers (deprecated)
├── docs/             # Documentation
├── test/             # Test scripts
└── README.md         # This file
```

**Note**: The current implementation is in the `cpp/` directory. The `src/` and `include/` directories contain an older C implementation that is no longer maintained.

## Limitations

- HTTP only (no HTTPS support)
- **Requires Large memory model (-ml)** - Medium (-mm) and Small (-ms) models are not supported
- No keep-alive connections
- Basic HTTP/1.0 and HTTP/1.1 support
- No cookie management
- No chunked transfer encoding
- Maximum response size: 64KB
- Maximum request size: 4KB

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
- The mTCP library is designed for Large memory model usage
- Attempting to use other models results in "Init: Could not setup packet buffers" error

**Build Configuration:**
The Makefile (`cpp/Makefile`) is pre-configured with Large memory model:
```makefile
memory_model = -ml
```
**Do not change this setting** - other memory models will not work.

## Known Issues

None currently. The program has been tested and works correctly in DOSBox-X with mTCP.

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