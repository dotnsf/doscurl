# DOSCurl - HTTP Client for DOS

A simple HTTP client for 16-bit DOS, built with mTCP.

## Features

- HTTP/1.0 GET requests
- DNS resolution
- Simple command-line interface
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

### Examples

```
doscurl http://www.brutman.com/
doscurl http://example.com/index.html
doscurl http://192.168.1.1:8080/api
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

- HTTP only (no HTTPS)
- GET requests only (POST support planned)
- No redirect following
- No chunked transfer encoding support yet
- Basic error handling

## License

Copyright (C) 2026

## Credits

- Built with mTCP by Michael B. Brutman
- Inspired by curl by Daniel Stenberg