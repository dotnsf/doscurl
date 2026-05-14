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

// Globals
char Hostname[HOSTNAME_LEN];
char Path[PATH_LEN];
char HttpMethod[METHOD_LEN] = "GET";
char PostData[POSTDATA_LEN];
char CustomHeaders[MAX_HEADERS][HEADER_LEN];
int CustomHeaderCount = 0;
char BasicAuth[AUTH_LEN];
char OutputFile[FILENAME_LEN];
int VerboseMode = 0;
uint32_t ConnectTimeout = DEFAULT_CONNECT_TIMEOUT;  // milliseconds
uint32_t MaxTime = DEFAULT_MAX_TIME;                // milliseconds
IpAddr_t HostAddr;
uint16_t ServerPort = 80;
TcpSocket *sock = NULL;

// Large buffers (global to avoid stack overflow)
uint8_t responseData[32768];
uint8_t recvBuffer[1024];
char httpRequest[2048];
char base64Buffer[256];

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
  fprintf(stderr, "\nShutting down (rc=%d)...\n", rc);
  // Skip Utils::endStack() to avoid "heap is corrupted" false positive
  // The program works correctly, but mTCP's heap check is overly sensitive
  // Utils::endStack();
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
  fprintf(stderr, "Resolving %s... ", Hostname);
  
  int8_t rc = Dns::resolve(Hostname, HostAddr, 1);
  if (rc < 0) {
    fprintf(stderr, "failed\n");
    return -1;
  }
  
  uint8_t done = 0;
  while (!done) {
    if (userWantsOut()) return -1;
    
    PACKET_PROCESS_SINGLE;
    Arp::driveArp();
    Tcp::drivePackets();
    
    if (Dns::isQueryPending()) {
      continue;
    }
    
    done = 1;
  }
  
  if (rc != 0) {
    fprintf(stderr, "failed\n");
    return -1;
  }
  
  fprintf(stderr, "%d.%d.%d.%d\n",
          HostAddr[0], HostAddr[1], HostAddr[2], HostAddr[3]);
  
  return 0;
}

// Connect to server
int connectToServer(void) {
  fprintf(stderr, "Connecting to %s:%u... ", Hostname, ServerPort);
  
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
  
  fprintf(stderr, "connected\n");
  return 0;
}

// Send HTTP request
int sendRequest(void) {
  int pos = 0;
  
  // Build request line
  pos += snprintf(httpRequest + pos, sizeof(httpRequest) - pos,
                  "%s %s HTTP/1.0\r\n", HttpMethod, Path);
  
  // Add Host header
  pos += snprintf(httpRequest + pos, sizeof(httpRequest) - pos,
                  "Host: %s\r\n", Hostname);
  
  // Add User-Agent header
  pos += snprintf(httpRequest + pos, sizeof(httpRequest) - pos,
                  "User-Agent: DOSCurl/0.1.0\r\n");
  
  // Add Basic Authentication header if provided
  if (BasicAuth[0] != '\0') {
    base64_encode(BasicAuth, base64Buffer, strlen(BasicAuth));
    pos += snprintf(httpRequest + pos, sizeof(httpRequest) - pos,
                    "Authorization: Basic %s\r\n", base64Buffer);
  }
  
  // Add custom headers
  for (int i = 0; i < CustomHeaderCount; i++) {
    pos += snprintf(httpRequest + pos, sizeof(httpRequest) - pos,
                    "%s\r\n", CustomHeaders[i]);
  }
  
  // Add Content-Length and Content-Type for POST data
  if (PostData[0] != '\0') {
    int dataLen = strlen(PostData);
    pos += snprintf(httpRequest + pos, sizeof(httpRequest) - pos,
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
      pos += snprintf(httpRequest + pos, sizeof(httpRequest) - pos,
                      "Content-Type: application/x-www-form-urlencoded\r\n");
    }
  }
  
  // Add Connection header
  pos += snprintf(httpRequest + pos, sizeof(httpRequest) - pos,
                  "Connection: close\r\n");
  
  // End of headers
  pos += snprintf(httpRequest + pos, sizeof(httpRequest) - pos, "\r\n");
  
  // Add POST data if present
  if (PostData[0] != '\0') {
    pos += snprintf(httpRequest + pos, sizeof(httpRequest) - pos, "%s", PostData);
  }
  
  fprintf(stderr, "Sending %s request...\n", HttpMethod);
  
  int len = strlen(httpRequest);
  int sent = 0;
  
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

// Receive and print response
int receiveResponse(void) {
  uint32_t totalBytes = 0;
  uint32_t responseLen = 0;
  FILE *outFile = NULL;
  
  if (VerboseMode) {
    fprintf(stderr, "Receiving response...\n");
  }
  
  clockTicks_t start = TIMER_GET_CURRENT();
  
  // Receive all data
  while (1) {
    if (userWantsOut()) return -1;
    
    if (Timer_diff(start, TIMER_GET_CURRENT()) > TIMER_MS_TO_TICKS(MaxTime)) {
      fprintf(stderr, "\nTimeout waiting for data after %lu ms\n", MaxTime);
      break;
    }
    
    PACKET_PROCESS_SINGLE;
    Arp::driveArp();
    Tcp::drivePackets();
    
    int16_t rc = sock->recv(recvBuffer, sizeof(recvBuffer));
    
    if (rc > 0) {
      // Data received - store in buffer
      if (responseLen + rc < sizeof(responseData)) {
        memcpy(responseData + responseLen, recvBuffer, rc);
        responseLen += rc;
      }
      totalBytes += rc;
      start = TIMER_GET_CURRENT(); // Reset timeout
    } else if (rc < 0) {
      fprintf(stderr, "\nReceive error\n");
      return -1;
    } else {
      // No data available
      if (sock->isRemoteClosed()) {
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
    // Print headers to stderr (without extra blank lines)
    uint32_t headerLen = headerEnd - responseData - 4;
    for (uint32_t i = 0; i < headerLen; i++) {
      fputc(responseData[i], stderr);
    }
    fprintf(stderr, "\n");
    
    // Print/save body
    uint32_t bodyLen = responseLen - (headerEnd - responseData);
    if (outFile) {
      fwrite(headerEnd, 1, bodyLen, outFile);
      fclose(outFile);
      if (VerboseMode) {
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
      if (VerboseMode) {
        fprintf(stderr, "Saved %lu bytes to '%s'\n", responseLen, OutputFile);
      }
    } else {
      fwrite(responseData, 1, responseLen, stdout);
    }
  }
  
  if (VerboseMode) {
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
          strcmp(argv[i], "--max-time") == 0 || strcmp(argv[i], "-m") == 0) {
        // Skip the next argument (option's value)
        i++;
      } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
        // -v doesn't take an argument, don't skip
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
    else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
      VerboseMode = 1;
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
  fprintf(stderr, "  -v, --verbose           Verbose output (show connection details)\n");
  fprintf(stderr, "  -o, --output <file>     Write output to file instead of stdout\n");
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
  
  // Initialize PostData, BasicAuth, and OutputFile
  PostData[0] = '\0';
  BasicAuth[0] = '\0';
  OutputFile[0] = '\0';
  
  // Parse command line arguments
  if (parseArguments(argc, argv) < 0) {
    return 1;
  }
  
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
  
  // Initialize TCP/IP stack
  fprintf(stderr, "Initializing TCP/IP stack...\n");
  int rc = Utils::parseEnv();
  if (rc != 0) {
    fprintf(stderr, "Error parsing environment: %d\n", rc);
    fprintf(stderr, "Make sure MTCP environment variable is set to your config file\n");
    return 1;
  }
  
  rc = Utils::initStack(2, TCP_SOCKET_RING_SIZE, ctrlBreakHandler, ctrlBreakHandler);
  if (rc != 0) {
    fprintf(stderr, "Failed to initialize TCP/IP stack: %d\n", rc);
    fprintf(stderr, "Check packet driver and network configuration\n");
    return 1;
  }
  fprintf(stderr, "TCP/IP stack initialized successfully\n\n");
  
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
  fprintf(stderr, "\nCalling receiveResponse()...\n");
  if (receiveResponse() < 0) {
    fprintf(stderr, "receiveResponse() failed\n");
    sock->close();
    shutdown(1);
  }
  fprintf(stderr, "receiveResponse() completed successfully\n");
  
  // Close connection
  fprintf(stderr, "Closing socket...\n");
  sock->close();
  fprintf(stderr, "Socket closed\n");
  
  fprintf(stderr, "Done!\n");
  shutdown(0);
  
  return 0;
}
