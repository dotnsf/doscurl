# DOSCurl - HTTP Client for DOS

A simple HTTP client for 16-bit DOS, built with mTCP.

## Features

- HTTP/1.0 and HTTP/1.1 support
- GET and POST requests
- Custom HTTP headers
- Basic authentication
- Redirect following (up to 10 redirects)
- Verbose output modes (curl-style)
- DNS resolution
- Configurable timeouts
- Save output to file
- Based on mTCP TCP/IP stack

## Requirements

- 16-bit DOS (PC-DOS, MS-DOS, FreeDOS, etc.)
- Packet driver loaded
- mTCP configuration file (MTCP.CFG)

## Building

### Prerequisites

- Open Watcom C/C++ v2 installed at `C:\WATCOM`
- mTCP source code at `C:\mTCP\src`

### Build Steps

```
cd cpp
build.bat
```

This will create `doscurl.exe`.

## Usage

### Basic Usage

```
doscurl http://example.com/
```

### Command Line Options

- `-X, --request <method>` - HTTP method (GET, POST, PUT, DELETE, etc.)
- `-d, --data <data>` - POST data
- `-H, --header <header>` - Custom HTTP header (can be used multiple times)
- `-u, --user <user:pass>` - Basic authentication
- `-o, --output <file>` - Save output to file
- `-L, --location` - Follow redirects (default: off)
- `--max-redirs <num>` - Maximum redirects (default: 10)
- `--connect-timeout <ms>` - Connection timeout in milliseconds (default: 10000)
- `--max-time <ms>` - Maximum time for entire operation (default: 30000)
- `-v` - Verbose mode: show HTTP request and response headers
- `-vv` - Extra verbose mode: show all debug messages
- `-h, --help` - Show help message

### Verbosity Levels

**Default (no option)**: Output HTTP response body only
```
doscurl http://example.com/
```

**`-v` option**: Show HTTP request (prefixed with `> `), response headers (prefixed with `< `), and body
```
doscurl -v http://example.com/
```
Output example:
```
> GET / HTTP/1.1
> Host: example.com
> User-Agent: DOSCurl/1.0
>
< HTTP/1.1 200 OK
< Content-Type: text/html
< Content-Length: 1234
<
<!DOCTYPE html>
<html>...
```

**`-vv` option**: Show all `-v` content plus debug messages (prefixed with `* `)
```
doscurl -vv http://example.com/
```
Output example:
```
* Method: GET
* Host: example.com
* Port: 80
* Path: /
*
* Resolving example.com... 93.184.216.34
* Connecting to 93.184.216.34:80...
* Connected
* Sending request...
> GET / HTTP/1.1
> Host: example.com
> User-Agent: DOSCurl/1.0
>
* Receiving response...
< HTTP/1.1 200 OK
< Content-Type: text/html
< Content-Length: 1234
<
<!DOCTYPE html>
<html>...
*
* Received 1234 bytes total
```

### Examples

**Simple GET request:**
```
doscurl http://www.brutman.com/
```

**GET with verbose output:**
```
doscurl -v http://example.com/index.html
```

**POST request with data:**
```
doscurl -X POST -d "name=value" http://example.com/api
```

**Custom headers:**
```
doscurl -H "Accept: application/json" -H "X-API-Key: secret" http://api.example.com/
```

**Basic authentication:**
```
doscurl -u username:password http://example.com/protected
```

**Save to file:**
```
doscurl -o output.html http://example.com/
```

**Follow redirects:**
```
doscurl -L http://example.com/redirect
```

**Custom timeout:**
```
doscurl --connect-timeout 5000 --max-time 15000 http://slow-server.com/
```

**Extra verbose debugging:**
```
doscurl -vv http://example.com/
```

## Configuration

DOSCurl uses mTCP, which requires a configuration file `MTCP.CFG` in the current directory or pointed to by the `MTCP_CFG` environment variable.

Example `MTCP.CFG`:

```
PACKETINT 0x60
IPADDR 192.168.1.100
NETMASK 255.255.255.0
GATEWAY 192.168.1.1
NAMESERVER 8.8.8.8
```

## Testing in DOSBox-X

1. Start DOSBox-X with NE2000 emulation
2. Load packet driver (e.g., `ne2000.com`)
3. Configure mTCP with `dhcp.exe` or create `MTCP.CFG`
4. Run `doscurl http://example.com/`

## Limitations

- HTTP only (no HTTPS support)
- No chunked transfer encoding support
- No compression support (gzip, deflate)
- Maximum response size: 64KB
- Maximum request size: 4KB

## License

Copyright (C) 2026

## Credits

- Built with mTCP by Michael B. Brutman
- Inspired by curl by Daniel Stenberg