# DOSCurl Testing Guide

## Prerequisites

1. **DOSBox-X** with NE2000 emulation configured
2. **mTCP** packet driver loaded
3. **MTCP.CFG** file configured with network settings
4. **HTTP test server** running (e.g., `python -m http.server 8080`)

## Setup Steps

### 1. Transfer doscurl.exe to DOSBox-X

Copy `doscurl.exe` to a directory accessible from DOSBox-X (e.g., mounted drive).

### 2. Verify mTCP Configuration

Ensure your `MTCP.CFG` file contains at least:

```
PACKETINT 0x60
IPADDR 192.168.1.100
NETMASK 255.255.255.0
GATEWAY 192.168.1.1
NAMESERVER 8.8.8.8
```

Adjust values according to your network setup.

### 3. Set Environment Variable

In DOSBox-X, set the MTCP configuration path:

```
SET MTCP=C:\MTCP.CFG
```

Or place `MTCP.CFG` in the same directory as `doscurl.exe`.

## Basic Tests

### Test 1: Local HTTP Server

If running Python HTTP server on host (192.168.1.1:8080):

```
doscurl http://192.168.1.1:8080/
```

Expected: HTML directory listing or index.html content

### Test 2: Specific File

```
doscurl http://192.168.1.1:8080/test.txt
```

Expected: Content of test.txt file

### Test 3: Remote Server (if internet accessible)

```
doscurl http://example.com/
```

Expected: HTML content from example.com

### Test 4: Non-standard Port

```
doscurl http://192.168.1.1:3000/api/data
```

Expected: Response from server on port 3000

## Troubleshooting

### Error: "Could not initialize TCP/IP stack"

- Check packet driver is loaded (`PKTDRV` or similar)
- Verify `MTCP` environment variable is set
- Ensure `MTCP.CFG` file exists and is readable

### Error: "DNS resolution failed"

- Check `NAMESERVER` in `MTCP.CFG`
- Try using IP address instead of hostname
- Verify network connectivity with mTCP's `ping` utility

### Error: "Connection failed"

- Verify target server is running and accessible
- Check firewall settings on host machine
- Ensure correct IP address and port number
- Test with mTCP's `telnet` utility first

### Error: "Connection timeout"

- Increase timeout value (currently 15 seconds)
- Check network latency
- Verify gateway configuration in `MTCP.CFG`

### No output or hangs

- Press Ctrl+C to abort
- Check if packet driver is responding
- Verify NE2000 emulation is working in DOSBox-X
- Try mTCP's diagnostic tools (`dhcp`, `ping`)

## Debug Mode

To enable verbose output, modify `doscurl.cpp` and rebuild:

```cpp
// Uncomment this line near the top of main()
// TRACE_ON = 1;
```

This will show detailed packet and connection information.

## Performance Notes

- First DNS lookup may take several seconds
- Connection establishment typically takes 1-3 seconds
- Large responses may take time due to 16-bit processing
- Ctrl+Break can interrupt long transfers

## Next Steps

Once basic HTTP GET is working:

1. Test with various content types (HTML, JSON, plain text)
2. Test with different response sizes
3. Implement POST functionality
4. Add command-line options for headers and methods
5. Improve error messages and response parsing

## Example Test Session

```
C:\>SET MTCP=C:\MTCP.CFG

C:\>doscurl http://192.168.1.1:8080/
Resolving 192.168.1.1...
Connecting to 192.168.1.1:8080...
Connected!
Sending HTTP request...
Receiving response...

HTTP/1.0 200 OK
Server: SimpleHTTP/0.6 Python/3.11.0
Date: Thu, 14 May 2026 03:20:00 GMT
Content-type: text/html; charset=utf-8
Content-Length: 1234

<!DOCTYPE html>
<html>
...
</html>

Connection closed.
```

## Common Test URLs

- `http://192.168.1.1:8080/` - Local Python HTTP server
- `http://example.com/` - Simple test page
- `http://httpbin.org/get` - HTTP testing service (returns JSON)
- `http://httpbin.org/status/200` - Returns specific status code
- `http://neverssl.com/` - Plain HTTP site (no redirect to HTTPS)

## Validation Checklist

- [ ] Program starts without errors
- [ ] DNS resolution works (or IP address accepted)
- [ ] TCP connection established
- [ ] HTTP request sent correctly
- [ ] Response headers received
- [ ] Response body displayed
- [ ] Connection closed cleanly
- [ ] Ctrl+Break interrupts properly
- [ ] Error messages are clear and helpful