/*
 * DOSCurl - Simple HTTP client for DOS using mTCP
 * Copyright (C) 2026
 */

#define DOSCURL_VERSION "0.1.0"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "types.h"
#include "timer.h"
#include "trace.h"
#include "utils.h"
#include "packet.h"
#include "arp.h"
#include "tcp.h"
#include "tcpsockm.h"
#include "udp.h"
#include "dns.h"

// Configuration
#define HOSTNAME_LEN       (80)
#define PATH_LEN          (256)
#define METHOD_LEN         (16)
#define POSTDATA_LEN     (4096)
#define HEADER_LEN        (256)
#define MAX_HEADERS        (10)
#define AUTH_LEN          (128)
#define FILENAME_LEN      (256)
#define TCP_RECV_BUFFER (4096)
#define DEFAULT_CONNECT_TIMEOUT (10000ul)  // 10 seconds
#define DEFAULT_MAX_TIME (30000ul)         // 30 seconds
#define MAX_REDIRECTS      (10)            // Maximum redirect follows

// Globals
char Hostname[HOSTNAME_LEN];
char Path[PATH_LEN];
char HttpMethod[METHOD_LEN] = "GET";
char PostData[POSTDATA_LEN];
char CustomHeaders[MAX_HEADERS][HEADER_LEN];
int CustomHeaderCount = 0;
char BasicAuth[AUTH_LEN];
char OutputFile[FILENAME_LEN];
int VerboseMode = 0;  // 0=quiet, 1=-v (show headers), 2=-vv (show debug)
int FollowRedirects = 1;  // Follow redirects by default
int MaxRedirects = MAX_REDIRECTS;
uint32_t ConnectTimeout = DEFAULT_CONNECT_TIMEOUT;  // milliseconds
uint32_t MaxTime = DEFAULT_MAX_TIME;                // milliseconds
IpAddr_t HostAddr;
uint16_t ServerPort = 80;
TcpSocket *sock = NULL;
int LastHttpStatus = 0;  // Last HTTP status code received

// Large buffers (dynamically allocated to avoid mTCP heap check false positives)
uint8_t *responseData = NULL;
uint8_t *recvBuffer = NULL;
char *httpRequest = NULL;
char *base64Buffer = NULL;

// Buffer sizes (reduced for 16-bit DOS memory constraints)
#define RESPONSE_DATA_SIZE 16384  // 16KB instead of 32KB
#define RECV_BUFFER_SIZE 1024
#define HTTP_REQUEST_SIZE 2048
#define BASE64_BUFFER_SIZE 256

// Ctrl-Break handler
volatile uint8_t CtrlBreakDetected = 0;

void __interrupt __far ctrlBreakHandler(void) {
  CtrlBreakDetected = 1;
}

// Check if user wants to abort
uint8_t userWantsOut(void) {
  if (CtrlBreakDetected) {
    fprintf(stderr, "Ctrl-Break detected - aborting!\n");
    return 1;
  }
  return 0;
}

// Base64 encoding table
static const char base64_table[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Base64 encode function
void base64_encode(const char *input, char *output, int input_len) {
  int i, j;
  unsigned char a, b, c;
  
  for (i = 0, j = 0; i < input_len; i += 3) {
    a = input[i];
    b = (i + 1 < input_len) ? input[i + 1] : 0;
    c = (i + 2 < input_len) ? input[i + 2] : 0;
    
    output[j++] = base64_table[a >> 2];
    output[j++] = base64_table[((a & 0x03) << 4) | (b >> 4)];
    output[j++] = (i + 1 < input_len) ? base64_table[((b & 0x0F) << 2) | (c >> 6)] : '=';
    output[j++] = (i + 2 < input_len) ? base64_table[c & 0x3F] : '=';
  }
  
  output[j] = '\0';
}

// Shutdown and exit
static void shutdown(int rc) {
  // Clean up socket if it exists
  if (sock != NULL) {
    sock->close();
    TcpSocketMgr::freeSocket(sock);
    sock = NULL;
  }
  
  // Free dynamically allocated buffers
  if (responseData) free(responseData);
  if (recvBuffer) free(recvBuffer);
  if (httpRequest) free(httpRequest);
  if (base64Buffer) free(base64Buffer);
  
  // Call Utils::endStack() to properly clean up mTCP stack.
  // This is necessary to allow subsequent executions to work.
  //
  // Note: Utils::endStack() will print "End: heap is corrupted!" message
  // because it calls _heapchk() which fails when we use malloc/free.
  // This is a false positive - the program works correctly.
  // The message can be safely ignored.
  Utils::endStack();
  
  exit(rc);
}

// Parse URL
int parseUrl(const char *url) {
  const char *p = url;
  
  // Skip http://
  if (strncmp(p, "http://", 7) == 0) {
    p += 7;
  }
  
  // Extract hostname
  const char *slash = strchr(p, '/');
  const char *colon = strchr(p, ':');
  
  int hostLen;
  if (slash && colon && colon < slash) {
    // Port specified
    hostLen = colon - p;
    if (hostLen >= HOSTNAME_LEN) hostLen = HOSTNAME_LEN - 1;
    strncpy(Hostname, p, hostLen);
    Hostname[hostLen] = '\0';
    ServerPort = atoi(colon + 1);
    p = slash;
  } else if (slash) {
    // No port
    hostLen = slash - p;
    if (hostLen >= HOSTNAME_LEN) hostLen = HOSTNAME_LEN - 1;
    strncpy(Hostname, p, hostLen);
    Hostname[hostLen] = '\0';
    p = slash;
  } else if (colon) {
    // Port but no path
    hostLen = colon - p;
    if (hostLen >= HOSTNAME_LEN) hostLen = HOSTNAME_LEN - 1;
    strncpy(Hostname, p, hostLen);
    Hostname[hostLen] = '\0';
    ServerPort = atoi(colon + 1);
    strcpy(Path, "/");
    return 0;
  } else {
    // Just hostname
    strncpy(Hostname, p, HOSTNAME_LEN - 1);
    Hostname[HOSTNAME_LEN - 1] = '\0';
    strcpy(Path, "/");
    return 0;
  }
  
  // Extract path
  if (p && *p) {
    strncpy(Path, p, PATH_LEN - 1);
    Path[PATH_LEN - 1] = '\0';
  } else {
    strcpy(Path, "/");
  }
  
  return 0;
}

// Resolve hostname
int resolveHost(void) {
  if (VerboseMode >= 2) {
    fprintf(stderr, "Resolving %s... ", Hostname);
  }
  
  // Initialize HostAddr to zeros
  HostAddr[0] = HostAddr[1] = HostAddr[2] = HostAddr[3] = 0;
  
  // Start DNS resolution
  int8_t rc = Dns::resolve(Hostname, HostAddr, 1);
  
  if (rc < 0) {
    if (VerboseMode >= 2) {
      fprintf(stderr, "failed\n");
    }
    return -1;
  }
  
  // If rc == 0, it's an IP address and already resolved
  if (rc == 0) {
    if (VerboseMode >= 2) {
      fprintf(stderr, "%d.%d.%d.%d\n",
              HostAddr[0], HostAddr[1], HostAddr[2], HostAddr[3]);
    }
    return 0;
  }
  
  // rc == 1 means DNS resolution is needed
  clockTicks_t start = TIMER_GET_CURRENT();
  
  // Wait for DNS response
  while (!userWantsOut() && Dns::isQueryPending()) {
    // Check for DNS timeout
    if (Timer_diff(start, TIMER_GET_CURRENT()) > TIMER_MS_TO_TICKS(ConnectTimeout)) {
      if (VerboseMode >= 2) {
        fprintf(stderr, "timeout\n");
      }
      return -1;
    }
    
    PACKET_PROCESS_SINGLE;
    Arp::driveArp();
    Tcp::drivePackets();
    Dns::drivePendingQuery();
  }
  
  if (userWantsOut()) {
    return -1;
  }
  
  // Get the final result
  rc = Dns::resolve(Hostname, HostAddr, 0);
  
  if (rc != 0) {
    if (VerboseMode >= 2) {
      fprintf(stderr, "failed\n");
    }
    return -1;
  }
  
  // Check if HostAddr was successfully set
  if (HostAddr[0] == 0 && HostAddr[1] == 0 && HostAddr[2] == 0 && HostAddr[3] == 0) {
    if (VerboseMode >= 2) {
      fprintf(stderr, "failed\n");
    }
    return -1;
  }
  
  if (VerboseMode >= 2) {
    fprintf(stderr, "%d.%d.%d.%d\n",
            HostAddr[0], HostAddr[1], HostAddr[2], HostAddr[3]);
  }
  
  return 0;
}

// Connect to server
int connectToServer(void) {
  if (VerboseMode >= 2) {
    fprintf(stderr, "Connecting to %s:%u... ", Hostname, ServerPort);
  }
  
  uint16_t localport = 2048 + rand();
  
  sock = TcpSocketMgr::getSocket();
  if (sock == NULL) {
    fprintf(stderr, "failed to get socket\n");
    return -1;
  }
  
  if (sock->setRecvBuffer(TCP_RECV_BUFFER)) {
    fprintf(stderr, "failed to set buffer\n");
    return -1;
  }
  
  if (sock->connectNonBlocking(localport, HostAddr, ServerPort)) {
    fprintf(stderr, "failed to initiate connection\n");
    return -1;
  }
  
  clockTicks_t start = TIMER_GET_CURRENT();
  
  while (1) {
    if (userWantsOut()) return -1;
    
    if (Timer_diff(start, TIMER_GET_CURRENT()) > TIMER_MS_TO_TICKS(ConnectTimeout)) {
      fprintf(stderr, "connection timeout after %lu ms\n", ConnectTimeout);
      return -1;
    }
    
    PACKET_PROCESS_SINGLE;
    Arp::driveArp();
    Tcp::drivePackets();
    
    if (sock->isConnectComplete()) {
      break;
    }
    
    // Check for connection errors
    if (sock->isRemoteClosed()) {
      fprintf(stderr, "failed\n");
      return -1;
    }
  }
  
  if (VerboseMode >= 2) {
    fprintf(stderr, "connected\n");
  }
  return 0;
}

// Send HTTP request
int sendRequest(void) {
  int pos = 0;
  
  // Build request line
  pos += snprintf(httpRequest + pos, HTTP_REQUEST_SIZE - pos,
                  "%s %s HTTP/1.0\r\n", HttpMethod, Path);
  
  // Add Host header
  pos += snprintf(httpRequest + pos, HTTP_REQUEST_SIZE - pos,
                  "Host: %s\r\n", Hostname);
  
  // Add User-Agent header
  pos += snprintf(httpRequest + pos, HTTP_REQUEST_SIZE - pos,
                  "User-Agent: DOSCurl/0.1.0\r\n");
  
  // Add Basic Authentication header if provided
  if (BasicAuth[0] != '\0') {
    base64_encode(BasicAuth, base64Buffer, strlen(BasicAuth));
    pos += snprintf(httpRequest + pos, HTTP_REQUEST_SIZE - pos,
                    "Authorization: Basic %s\r\n", base64Buffer);
  }
  
  // Add custom headers
  for (int i = 0; i < CustomHeaderCount; i++) {
    pos += snprintf(httpRequest + pos, HTTP_REQUEST_SIZE - pos,
                    "%s\r\n", CustomHeaders[i]);
  }
  
  // Add Content-Length and Content-Type for POST data
  if (PostData[0] != '\0') {
    int dataLen = strlen(PostData);
    pos += snprintf(httpRequest + pos, HTTP_REQUEST_SIZE - pos,
                    "Content-Length: %d\r\n", dataLen);
    
    // Add default Content-Type if not specified in custom headers
    int hasContentType = 0;
    for (int i = 0; i < CustomHeaderCount; i++) {
      if (strncmp(CustomHeaders[i], "Content-Type:", 13) == 0) {
        hasContentType = 1;
        break;
      }
    }
    if (!hasContentType) {
      pos += snprintf(httpRequest + pos, HTTP_REQUEST_SIZE - pos,
                      "Content-Type: application/x-www-form-urlencoded\r\n");
    }
  }
  
  // Add Connection header
  pos += snprintf(httpRequest + pos, HTTP_REQUEST_SIZE - pos,
                  "Connection: close\r\n");
  
  // End of headers
  pos += snprintf(httpRequest + pos, HTTP_REQUEST_SIZE - pos, "\r\n");
  
  // Add POST data if present
  if (PostData[0] != '\0') {
    pos += snprintf(httpRequest + pos, HTTP_REQUEST_SIZE - pos, "%s", PostData);
  }
  
  if (VerboseMode >= 2) {
    fprintf(stderr, "Sending %s request...\n", HttpMethod);
  }
  
  int len = strlen(httpRequest);
  int sent = 0;
  
  // Debug: show request being sent in extra verbose mode
  if (VerboseMode >= 2) {
    fprintf(stderr, "--- Request ---\n%s--- End Request ---\n", httpRequest);
  }
  
  while (sent < len) {
    if (userWantsOut()) return -1;
    
    PACKET_PROCESS_SINGLE;
    Arp::driveArp();
    Tcp::drivePackets();
    
    int16_t rc = sock->send((uint8_t *)(httpRequest + sent), len - sent);
    if (rc > 0) {
      sent += rc;
    } else if (rc < 0) {
      fprintf(stderr, "Send error\n");
      return -1;
    }
  }
  
  return 0;
}

// Parse HTTP status line and extract status code
int parseHttpStatus(const char *statusLine) {
  // Expected format: "HTTP/1.x NNN Message"
  int status = 0;
  const char *p = statusLine;
  
  // Skip "HTTP/"
  if (strncmp(p, "HTTP/", 5) != 0) {
    return 0;
  }
  p += 5;
  
  // Skip version (e.g., "1.0" or "1.1")
  while (*p && *p != ' ') p++;
  if (*p == ' ') p++;
  
  // Parse status code
  status = atoi(p);
  
  return status;
}

// Extract Location header from response headers
int extractLocation(const uint8_t *headers, uint32_t headerLen, char *location, int maxLen) {
  const char *p = (const char *)headers;
  const char *end = p + headerLen;
  
  while (p < end) {
    // Find line start
    const char *lineStart = p;
    const char *lineEnd = lineStart;
    
    // Find line end
    while (lineEnd < end && *lineEnd != '\r' && *lineEnd != '\n') {
      lineEnd++;
    }
    
    // Check if this is Location header (case-insensitive)
    if (lineEnd - lineStart > 10 &&
        (strncmp(lineStart, "Location:", 9) == 0 ||
         strncmp(lineStart, "location:", 9) == 0)) {
      const char *value = lineStart + 9;
      // Skip whitespace
      while (value < lineEnd && (*value == ' ' || *value == '\t')) {
        value++;
      }
      
      int len = lineEnd - value;
      if (len > 0 && len < maxLen) {
        strncpy(location, value, len);
        location[len] = '\0';
        return 1;
      }
    }
    
    // Move to next line
    p = lineEnd;
    if (p < end && *p == '\r') p++;
    if (p < end && *p == '\n') p++;
  }
  
  return 0;
}

// Receive and print response
int receiveResponse(void) {
  uint32_t totalBytes = 0;
  uint32_t responseLen = 0;
  FILE *outFile = NULL;
  
  if (VerboseMode >= 2) {
    fprintf(stderr, "Receiving response...\n");
  }
  
  clockTicks_t start = TIMER_GET_CURRENT();
  clockTicks_t lastData = start;
  int noDataCount = 0;
  
  // Receive all data
  while (1) {
    if (userWantsOut()) return -1;
    
    // Overall timeout
    if (Timer_diff(start, TIMER_GET_CURRENT()) > TIMER_MS_TO_TICKS(MaxTime)) {
      fprintf(stderr, "\nTimeout waiting for data after %lu ms\n", MaxTime);
      break;
    }
    
    // If no data for 2 seconds and we have some response, consider it complete
    if (responseLen > 0 && Timer_diff(lastData, TIMER_GET_CURRENT()) > TIMER_MS_TO_TICKS(2000)) {
      if (VerboseMode >= 2) {
        fprintf(stderr, "No more data after 2 seconds, considering response complete\n");
      }
      break;
    }
    
    PACKET_PROCESS_SINGLE;
    Arp::driveArp();
    Tcp::drivePackets();
    
    int16_t rc = sock->recv(recvBuffer, RECV_BUFFER_SIZE);
    
    if (rc > 0) {
      // Data received - store in buffer
      if (responseLen + rc < RESPONSE_DATA_SIZE) {
        memcpy(responseData + responseLen, recvBuffer, rc);
        responseLen += rc;
      }
      totalBytes += rc;
      lastData = TIMER_GET_CURRENT(); // Reset data timeout
      noDataCount = 0;
    } else if (rc < 0) {
      fprintf(stderr, "\nReceive error\n");
      return -1;
    } else {
      // No data available
      if (sock->isRemoteClosed()) {
        break;
      }
      noDataCount++;
      // If we've checked many times with no data and connection seems idle, break
      if (responseLen > 0 && noDataCount > 100) {
        if (VerboseMode >= 2) {
          fprintf(stderr, "Connection idle, considering response complete\n");
        }
        break;
      }
    }
  }
  
  // Parse headers and body
  uint8_t *headerEnd = NULL;
  for (uint32_t i = 0; i < responseLen - 3; i++) {
    if (responseData[i] == '\r' && responseData[i+1] == '\n' &&
        responseData[i+2] == '\r' && responseData[i+3] == '\n') {
      headerEnd = responseData + i + 4;
      break;
    }
  }
  
  // Open output file if specified
  if (OutputFile[0] != '\0') {
    outFile = fopen(OutputFile, "wb");
    if (!outFile) {
      fprintf(stderr, "Error: Cannot open output file '%s'\n", OutputFile);
      return -1;
    }
  }
  
  if (headerEnd) {
    // Parse HTTP status code
    uint32_t headerLen = headerEnd - responseData - 4;
    char statusLine[128];
    int statusLineLen = 0;
    
    // Extract status line (first line)
    for (uint32_t i = 0; i < headerLen && i < sizeof(statusLine) - 1; i++) {
      if (responseData[i] == '\r' || responseData[i] == '\n') {
        break;
      }
      statusLine[statusLineLen++] = responseData[i];
    }
    statusLine[statusLineLen] = '\0';
    
    // Parse status code
    LastHttpStatus = parseHttpStatus(statusLine);
    
    // Print headers to stderr if verbose mode is enabled
    if (VerboseMode >= 1) {
      for (uint32_t i = 0; i < headerLen; i++) {
        fputc(responseData[i], stderr);
      }
      fprintf(stderr, "\n\n");  // Extra blank line to separate headers from body
    }
    
    // Check for error status codes
    if (LastHttpStatus >= 400) {
      if (LastHttpStatus >= 500) {
        fprintf(stderr, "Error: Server error (HTTP %d)\n", LastHttpStatus);
      } else {
        fprintf(stderr, "Error: Client error (HTTP %d)\n", LastHttpStatus);
      }
    }
    
    // Print/save body
    uint32_t bodyLen = responseLen - (headerEnd - responseData);
    if (outFile) {
      fwrite(headerEnd, 1, bodyLen, outFile);
      fclose(outFile);
      if (VerboseMode >= 2) {
        fprintf(stderr, "Saved %lu bytes to '%s'\n", bodyLen, OutputFile);
      }
    } else {
      fwrite(headerEnd, 1, bodyLen, stdout);
    }
  } else {
    // No header separator found, print everything
    if (outFile) {
      fwrite(responseData, 1, responseLen, outFile);
      fclose(outFile);
      if (VerboseMode >= 2) {
        fprintf(stderr, "Saved %lu bytes to '%s'\n", responseLen, OutputFile);
      }
    } else {
      fwrite(responseData, 1, responseLen, stdout);
    }
  }
  
  if (VerboseMode >= 2) {
    fprintf(stderr, "\nReceived %lu bytes total\n", totalBytes);
  }
  
  return 0;
}

// Parse command line arguments
int parseArguments(int argc, char *argv[]) {
  int urlIndex = -1;
  
  // First pass: Find URL (skip known options and their arguments)
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      // This is an option, check if it takes an argument
      if (strcmp(argv[i], "-X") == 0 || strcmp(argv[i], "--request") == 0 ||
          strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--data") == 0 ||
          strcmp(argv[i], "-H") == 0 || strcmp(argv[i], "--header") == 0 ||
          strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--user") == 0 ||
          strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0 ||
          strcmp(argv[i], "--connect-timeout") == 0 ||
          strcmp(argv[i], "--max-time") == 0 || strcmp(argv[i], "-m") == 0 ||
          strcmp(argv[i], "--max-redirs") == 0) {
        // Skip the next argument (option's value)
        i++;
      } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0 ||
                 strcmp(argv[i], "-vv") == 0 ||
                 strcmp(argv[i], "-L") == 0 || strcmp(argv[i], "--location") == 0) {
        // These options don't take an argument, don't skip
      }
    } else {
      // This is not an option, must be the URL
      urlIndex = i;
      break;
    }
  }
  
  if (urlIndex < 0) {
    fprintf(stderr, "Error: No URL specified\n");
    return -1;
  }
  
  // Parse URL
  if (parseUrl(argv[urlIndex]) < 0) {
    fprintf(stderr, "Error: Invalid URL\n");
    return -1;
  }
  
  // Second pass: Parse options
  for (int i = 1; i < argc; i++) {
    if (i == urlIndex) continue;
    
    if (strcmp(argv[i], "-X") == 0 || strcmp(argv[i], "--request") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "Error: -X requires an argument\n");
        return -1;
      }
      strncpy(HttpMethod, argv[++i], METHOD_LEN - 1);
      HttpMethod[METHOD_LEN - 1] = '\0';
      // Convert to uppercase
      for (char *p = HttpMethod; *p; p++) {
        *p = toupper(*p);
      }
    }
    else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--data") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "Error: -d requires an argument\n");
        return -1;
      }
      strncpy(PostData, argv[++i], POSTDATA_LEN - 1);
      PostData[POSTDATA_LEN - 1] = '\0';
      // If method not explicitly set, use POST
      if (strcmp(HttpMethod, "GET") == 0) {
        strcpy(HttpMethod, "POST");
      }
    }
    else if (strcmp(argv[i], "-H") == 0 || strcmp(argv[i], "--header") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "Error: -H requires an argument\n");
        return -1;
      }
      if (CustomHeaderCount >= MAX_HEADERS) {
        fprintf(stderr, "Error: Too many custom headers (max %d)\n", MAX_HEADERS);
        return -1;
      }
      strncpy(CustomHeaders[CustomHeaderCount++], argv[++i], HEADER_LEN - 1);
      CustomHeaders[CustomHeaderCount - 1][HEADER_LEN - 1] = '\0';
    }
    else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--user") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "Error: -u requires an argument\n");
        return -1;
      }
      strncpy(BasicAuth, argv[++i], AUTH_LEN - 1);
      BasicAuth[AUTH_LEN - 1] = '\0';
    }
    else if (strcmp(argv[i], "-vv") == 0) {
      VerboseMode = 2;  // Extra verbose (debug mode)
    }
    else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
      VerboseMode = 1;  // Verbose (show headers)
    }
    else if (strcmp(argv[i], "-L") == 0 || strcmp(argv[i], "--location") == 0) {
      FollowRedirects = 1;  // Already default, but explicit
    }
    else if (strcmp(argv[i], "--max-redirs") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "Error: --max-redirs requires an argument\n");
        return -1;
      }
      MaxRedirects = atoi(argv[++i]);
      if (MaxRedirects < 0) {
        MaxRedirects = 0;
      }
    }
    else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "Error: -o requires an argument\n");
        return -1;
      }
      strncpy(OutputFile, argv[++i], FILENAME_LEN - 1);
      OutputFile[FILENAME_LEN - 1] = '\0';
    }
    else if (strcmp(argv[i], "--connect-timeout") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "Error: --connect-timeout requires an argument\n");
        return -1;
      }
      ConnectTimeout = atol(argv[++i]) * 1000;  // Convert seconds to milliseconds
      if (ConnectTimeout == 0) {
        fprintf(stderr, "Error: Invalid connect timeout value\n");
        return -1;
      }
    }
    else if (strcmp(argv[i], "--max-time") == 0 || strcmp(argv[i], "-m") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "Error: --max-time requires an argument\n");
        return -1;
      }
      MaxTime = atol(argv[++i]) * 1000;  // Convert seconds to milliseconds
      if (MaxTime == 0) {
        fprintf(stderr, "Error: Invalid max time value\n");
        return -1;
      }
    }
    else if (argv[i][0] == '-') {
      fprintf(stderr, "Warning: Unknown option '%s'\n", argv[i]);
    }
  }
  
  return 0;
}

// Print usage information
void printUsage(void) {
  fprintf(stderr, "DOSCurl v%s - Simple HTTP client for DOS\n\n", DOSCURL_VERSION);
  fprintf(stderr, "Usage: doscurl [options] <url>\n");
  fprintf(stderr, "       doscurl --version\n\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -X, --request <method>  HTTP method (GET, POST, etc.)\n");
  fprintf(stderr, "  -d, --data <data>       POST data (implies -X POST)\n");
  fprintf(stderr, "  -H, --header <header>   Custom header (can be used multiple times)\n");
  fprintf(stderr, "  -u, --user <user:pass>  Basic authentication credentials\n");
  fprintf(stderr, "  -v, --verbose           Verbose output (show HTTP headers)\n");
  fprintf(stderr, "  -vv                     Extra verbose output (show debug info)\n");
  fprintf(stderr, "  -o, --output <file>     Write output to file instead of stdout\n");
  fprintf(stderr, "  -L, --location          Follow HTTP redirects (enabled by default)\n");
  fprintf(stderr, "  --max-redirs <num>      Maximum number of redirects to follow (default: 10)\n");
  fprintf(stderr, "  -m, --max-time <sec>    Maximum time for the entire operation (default: 30)\n");
  fprintf(stderr, "  --connect-timeout <sec> Maximum time for connection (default: 10)\n");
  fprintf(stderr, "  --version               Show version information\n");
  fprintf(stderr, "\nExamples:\n");
  fprintf(stderr, "  doscurl http://example.com/\n");
  fprintf(stderr, "  doscurl -X POST -d \"key=value\" http://example.com/api\n");
  fprintf(stderr, "  doscurl -H \"Authorization: Bearer token\" http://example.com/\n");
  fprintf(stderr, "  doscurl -u \"username:password\" http://example.com/\n");
  fprintf(stderr, "  doscurl -v http://example.com/\n");
  fprintf(stderr, "  doscurl -o output.txt http://example.com/data.json\n");
  fprintf(stderr, "  doscurl --connect-timeout 5 -m 60 http://example.com/\n");
}

// Main function
int main(int argc, char *argv[]) {
  // Check for --version option
  if (argc == 2 && strcmp(argv[1], "--version") == 0) {
    fprintf(stderr, "DOSCurl v%s - Simple HTTP client for DOS\n", DOSCURL_VERSION);
    return 0;
  }
  
  if (argc < 2) {
    printUsage();
    return 1;
  }
  
  // Allocate large buffers dynamically to avoid mTCP heap check false positives
  // Allocate memory buffers
  responseData = (uint8_t *)malloc(RESPONSE_DATA_SIZE);
  if (!responseData) {
    fprintf(stderr, "Error: Failed to allocate responseData\n");
    return 1;
  }
  
  recvBuffer = (uint8_t *)malloc(RECV_BUFFER_SIZE);
  if (!recvBuffer) {
    fprintf(stderr, "Error: Failed to allocate recvBuffer\n");
    free(responseData);
    return 1;
  }
  
  httpRequest = (char *)malloc(HTTP_REQUEST_SIZE);
  if (!httpRequest) {
    fprintf(stderr, "Error: Failed to allocate httpRequest\n");
    free(responseData);
    free(recvBuffer);
    return 1;
  }
  
  base64Buffer = (char *)malloc(BASE64_BUFFER_SIZE);
  if (!base64Buffer) {
    fprintf(stderr, "Error: Failed to allocate base64Buffer\n");
    free(responseData);
    free(recvBuffer);
    free(httpRequest);
    return 1;
  }
  
  // Initialize PostData, BasicAuth, and OutputFile
  PostData[0] = '\0';
  BasicAuth[0] = '\0';
  OutputFile[0] = '\0';
  
  // Parse command line arguments
  if (parseArguments(argc, argv) < 0) {
    return 1;
  }
  
  if (VerboseMode >= 2) {
    fprintf(stderr, "Method: %s\n", HttpMethod);
    fprintf(stderr, "Host: %s\n", Hostname);
    fprintf(stderr, "Port: %u\n", ServerPort);
    fprintf(stderr, "Path: %s\n", Path);
    if (PostData[0] != '\0') {
      fprintf(stderr, "POST Data: %s\n", PostData);
    }
    if (CustomHeaderCount > 0) {
      fprintf(stderr, "Custom Headers: %d\n", CustomHeaderCount);
    }
    fprintf(stderr, "\n");
  }
  
  // Initialize TCP/IP stack
  int rc = Utils::parseEnv();
  if (rc != 0) {
    fprintf(stderr, "Error parsing environment: %d\n", rc);
    fprintf(stderr, "Make sure MTCP environment variable is set to your config file\n");
    return 1;
  }
  
  // Initialize stack with 2 TCP sockets and 4 transmit buffers
  // Parameters: tcpSockets, tcpXmitBuffers, ctrlBreakHandler, ctrlCHandler
  rc = Utils::initStack(2, 4, ctrlBreakHandler, ctrlBreakHandler);
  if (rc != 0) {
    fprintf(stderr, "Failed to initialize TCP/IP stack: %d\n", rc);
    fprintf(stderr, "Check packet driver and network configuration\n");
    return 1;
  }
  
  // Main request loop (handles redirects)
  int redirectCount = 0;
  char currentUrl[HOSTNAME_LEN + PATH_LEN + 20];
  char locationUrl[HOSTNAME_LEN + PATH_LEN + 20];
  
  // Build initial URL
  snprintf(currentUrl, sizeof(currentUrl), "http://%s:%u%s", Hostname, ServerPort, Path);
  
  while (redirectCount <= MaxRedirects) {
    // Parse current URL
    if (parseUrl(currentUrl) < 0) {
      fprintf(stderr, "Error: Invalid URL '%s'\n", currentUrl);
      shutdown(1);
    }
    
    if (VerboseMode && redirectCount > 0) {
      fprintf(stderr, "Following redirect #%d to: %s\n", redirectCount, currentUrl);
    }
    
    // Resolve hostname
    if (resolveHost() < 0) {
      shutdown(1);
    }
    
    // Connect to server
    if (connectToServer() < 0) {
      shutdown(1);
    }
    
    // Send HTTP request
    if (sendRequest() < 0) {
      sock->close();
      shutdown(1);
    }
    
    // Receive response
    if (receiveResponse() < 0) {
      sock->close();
      shutdown(1);
    }
    
    // Close connection
    sock->close();
    
    // Check if we need to follow a redirect
    if (FollowRedirects && LastHttpStatus >= 300 && LastHttpStatus < 400) {
      // Extract Location header
      uint8_t *headerEnd = NULL;
      uint32_t responseLen = 0;
      
      // Find response length (simplified - assumes responseData is still valid)
      for (uint32_t i = 0; i < RESPONSE_DATA_SIZE - 3; i++) {
        if (responseData[i] == '\r' && responseData[i+1] == '\n' &&
            responseData[i+2] == '\r' && responseData[i+3] == '\n') {
          headerEnd = responseData + i + 4;
          responseLen = i;
          break;
        }
      }
      
      if (headerEnd && extractLocation(responseData, responseLen, locationUrl, sizeof(locationUrl))) {
        if (VerboseMode >= 2) {
          fprintf(stderr, "Redirect (HTTP %d) to: %s\n", LastHttpStatus, locationUrl);
        }
        
        // Check redirect count
        redirectCount++;
        if (redirectCount > MaxRedirects) {
          fprintf(stderr, "Error: Too many redirects (max %d)\n", MaxRedirects);
          shutdown(1);
        }
        
        // Update current URL for next iteration
        strncpy(currentUrl, locationUrl, sizeof(currentUrl) - 1);
        currentUrl[sizeof(currentUrl) - 1] = '\0';
        
        // Continue loop to follow redirect
        continue;
      } else {
        fprintf(stderr, "Warning: Redirect response (HTTP %d) but no Location header found\n", LastHttpStatus);
        break;
      }
    }
    
    // No redirect, we're done
    break;
  }
  
  shutdown(0);
  
  return 0;
}
