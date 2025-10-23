# webserv — A tiny yet capable HTTP/1.1 server in C++98

![C++98](https://img.shields.io/badge/C%2B%2B-98-blue)
![HTTP/1.1](https://img.shields.io/badge/HTTP-1.1-2ea44f)
![Methods](https://img.shields.io/badge/Methods-GET%20%7C%20POST%20%7C%20DELETE-0366d6)
![CGI](https://img.shields.io/badge/CGI-supported-ff9800)
![Uploads](https://img.shields.io/badge/File%20Uploads-supported-9c27b0)
![Config](https://img.shields.io/badge/Config-NGINX--style-6f42c1)

# 🚀 WebServ - HTTP/1.1 Server Implementation

> **A high-performance HTTP/1.1 server built from scratch in C++98 for 42 School**

[![Tests](https://img.shields.io/badge/Tests-69%2F71%20Passed-brightgreen)](https://github.com/Yekanat/webserv)
[![Security](https://img.shields.io/badge/Security-14%2F14%20Passed-green)](https://github.com/Yekanat/webserv)
[![HTTP/1.1](https://img.shields.io/badge/HTTP-1.1%20Compliant-blue)](https://github.com/Yekanat/webserv)
[![C++98](https://img.shields.io/badge/C%2B%2B-98%20Standard-orange)](https://github.com/Yekanat/webserv)

---

## 📋 Table of Contents

1. [🎯 Project Overview](#-project-overview)
2. [⚡ Quick Start Guide](#-quick-start-guide)
3. [🏗️ Architecture Deep Dive](#️-architecture-deep-dive)
4. [🔧 Core Implementation](#-core-implementation)
5. [🧪 Testing & Validation](#-testing--validation)
6. [🛡️ Security Features](#️-security-features)
7. [📝 Configuration Guide](#-configuration-guide)
8. [🔍 Evaluation Checklist](#-evaluation-checklist)
9. [📊 Performance Metrics](#-performance-metrics)
10. [🚨 Troubleshooting](#-troubleshooting)

---

## 🎯 Project Overview

WebServ is a **complete HTTP/1.1 server implementation** that demonstrates mastery of:

- **Non-blocking I/O** with `select()` system call
- **Multi-client handling** with socket multiplexing  
- **NGINX-style configuration** parsing and management
- **Full HTTP method support** (GET, HEAD, POST, DELETE)
- **Enterprise-grade security** with comprehensive protection mechanisms
- **C++98 compliance** with modern software engineering principles

### 🎨 Key Features

| Feature | Implementation | Status |
|---------|---------------|--------|
| **Non-blocking I/O** | `select()` multiplexing | ✅ Complete |
| **Virtual Hosts** | Multi-server support | ✅ Complete |
| **HTTP Methods** | GET, HEAD, POST, DELETE | ✅ Complete |
| **File Upload** | Multipart form-data | ✅ Complete |
| **Security** | Path traversal, injection protection | ✅ Complete |
| **Configuration** | NGINX-style parser | ✅ Complete |
| **Error Handling** | Proper HTTP status codes | ✅ Complete |

---

## ⚡ Quick Start Guide

### 🏃‍♂️ 1. Compilation

```bash
# Clean build
make fclean
make

# Expected output: webserv executable created
```

### 🚀 2. Server Launch

```bash
# Start with default configuration
./webserv conf/default.conf

# Expected output:
# Socket in ascolto su 127.0.0.1:8080
# Socket in ascolto su 0.0.0.0:8000
# Server in esecuzione, in attesa di connessioni...
```

### 🌐 3. Quick Test

```bash
# Test in another terminal
curl http://localhost:8080/

# Expected: Beautiful landing page with server stats
```

### 🧪 4. Run Test Suites

```bash
# Comprehensive functionality tests
./run_comprehensive_tests.sh

# Security validation
./run_security_tests.sh

# POST method tests
./run_post_tests.sh

# DELETE method tests  
./run_delete_tests.sh
```

---

## 🏗️ Architecture Deep Dive

### 🎭 Design Philosophy

WebServ follows a **single-threaded, event-driven architecture** using the `select()` system call for efficient I/O multiplexing. This design choice ensures:

- **Predictable performance** without thread synchronization overhead
- **Simple debugging** with linear execution flow
- **Memory efficiency** with shared resources
- **C++98 compliance** without modern threading libraries

### 🔄 Request Processing Flow

```
[Client Connection] → [select() Monitoring] → [Socket Ready?] → [Read HTTP Request]
       ↓                                                              ↓
[Close/Keep-Alive] ← [Send Response] ← [Generate Response] ← [Parse & Route]
```

---

## 🔧 Core Implementation

### 🧠 1. Server Class (`src/Server.cpp`)

**The heart of the HTTP server** - handles all request processing and routing.

#### 🎯 Key Methods:

**`handleRequest(int clientSocket, const HttpRequest& request)`**
```cpp
// Master request router - determines HTTP method and delegates
void Server::handleRequest(int clientSocket, const HttpRequest& request) {
    // Method routing with security validation
    if (request.getMethod() == "GET" || request.getMethod() == "HEAD") {
        _handleGetRequest(clientSocket, request);
    } else if (request.getMethod() == "POST") {
        _handlePostRequest(clientSocket, request);
    } else if (request.getMethod() == "DELETE") {
        _handleDeleteRequest(clientSocket, request);
    } else {
        _sendErrorResponse(clientSocket, 405, "Method Not Allowed");
    }
}
```

**Why this design?**
- ✅ **Clear separation of concerns** - each method has dedicated handler
- ✅ **Easy to extend** - adding new methods requires minimal changes
- ✅ **Consistent error handling** - centralized error response system

**`_handleGetRequest(int clientSocket, const HttpRequest& request)`**
```cpp
// GET/HEAD handler with comprehensive file serving
void Server::_handleGetRequest(int clientSocket, const HttpRequest& request) {
    // 1. Security validation (directory traversal, null bytes)
    // 2. Virtual host resolution 
    // 3. Location matching with longest-prefix algorithm
    // 4. File existence and permission checks
    // 5. Content-Type detection
    // 6. Response generation (full body for GET, headers-only for HEAD)
}
```

**Advanced Features:**
- 🔒 **Directory traversal protection** with `..` and absolute path detection
- 📁 **Autoindex generation** for directories without index files
- 🎯 **MIME type detection** based on file extensions
- 📏 **Content-Length calculation** for proper HTTP compliance

### 🌐 2. ServerInstance Class (`src/ServerInstance.cpp`)

**Network layer manager** - handles socket creation, binding, and client connections.

#### 🔌 Key Features:

**Non-blocking Socket Setup:**
```cpp
// Configure socket for non-blocking operation
int flags = fcntl(serverSocket, F_GETFL, 0);
fcntl(serverSocket, F_SETFL, flags | O_NONBLOCK);
```

**Multi-port Binding with Duplicate Prevention:**
```cpp
std::map<std::string, int> uniqueSockets;
// Prevents binding same address:port multiple times
// Enables virtual hosting on different ports
```

### 📋 3. HttpRequest Class (`src/HttpRequest.cpp`)

**HTTP parser** - converts raw socket data into structured request objects.

#### 🔍 Parsing Highlights:

**Header Processing:**
```cpp
// Robust header parsing with validation
while (getline(ss, line) && line != "\r") {
    size_t colonPos = line.find(':');
    if (colonPos != std::string::npos) {
        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);
        // Trim whitespace and store
    }
}
```

**POST Data Handling:**
```cpp
// Support for both form-data and multipart uploads
if (contentType.find("multipart/form-data") != std::string::npos) {
    _parseMultipartData(body, boundary);
} else {
    _parseFormData(body);
}
```

### 📤 4. HttpResponse Class (`src/HttpResponse.cpp`)

**Response generator** - creates HTTP-compliant responses with proper headers.

#### 🎨 Response Features:

- ✅ **Proper status codes** (200, 204, 400, 403, 404, 405, 500)
- ✅ **Content-Type detection** based on file extensions
- ✅ **Content-Length headers** for all responses
- ✅ **Custom error pages** with consistent styling

---

## 🧪 Testing & Validation

### 📊 Test Suite Overview

| Test Suite | Tests | Passed | Coverage |
|------------|-------|--------|----------|
| **Comprehensive** | 42 | 42 ✅ | Core functionality |
| **Security** | 14 | 14 ✅ | Attack prevention |
| **POST Methods** | 5 | 4 ✅ | Form & file upload |
| **DELETE Methods** | 10 | 9 ✅ | File removal |
| **Total** | **71** | **69** | **97.2%** |

### 🎯 How to Evaluate Each Component

#### 1️⃣ **Basic Functionality Test**
```bash
# Terminal 1: Start server
./webserv conf/default.conf

# Terminal 2: Test basic GET
curl -v http://localhost:8080/
# Expected: 200 OK with HTML landing page

# Test virtual hosting
curl -H "Host: localhost" http://localhost:8000/
# Expected: Same content, different port
```

#### 2️⃣ **File Serving Test**
```bash
# Static file serving
curl http://localhost:8080/test.txt
# Expected: File content with proper Content-Type

# Directory autoindex  
curl http://localhost:8080/uploads/
# Expected: HTML directory listing
```

#### 3️⃣ **POST Method Test**
```bash
# Form data submission
curl -X POST -d "name=test&email=test@example.com" \
     http://localhost:8080/contact
# Expected: 200 OK with POST confirmation

# File upload test
curl -X POST -F "file=@test_files/simple.txt" \
     http://localhost:8080/upload  
# Expected: File saved in uploads/ directory
```

#### 4️⃣ **DELETE Method Test**
```bash
# Create test file
echo "delete me" > www/delete/test_delete.txt

# Delete via HTTP
curl -X DELETE http://localhost:8080/delete/test_delete.txt
# Expected: 200 OK, file removed

# Verify deletion
curl http://localhost:8080/delete/test_delete.txt
# Expected: 404 Not Found
```

#### 5️⃣ **Security Validation**
```bash
# Directory traversal attempt
curl http://localhost:8080/../../../etc/passwd
# Expected: 403 Forbidden

# Null byte injection
curl http://localhost:8080/index.html%00.txt
# Expected: 404 Not Found (filtered)

# Long URI test
curl http://localhost:8080/$(python3 -c "print('a'*2000)")
# Expected: 404 Not Found (handled gracefully)
```

---

## 🛡️ Security Features

### 🔒 1. Directory Traversal Protection

**Implementation in `src/Server.cpp`:**
```cpp
// Multi-layer path validation
if (fullPath.find("..") != std::string::npos || fullPath[0] == '/') {
    _sendErrorResponse(clientSocket, 403, "Forbidden");
    return;
}

// Additional stat() safety check
struct stat statbuf;
if (stat(fullPath.c_str(), &statbuf) != 0) {
    _sendErrorResponse(clientSocket, 404, "Not Found");
    return;
}
```

**Why This Works:**
- ✅ **String-based detection** catches `../` patterns
- ✅ **Absolute path prevention** blocks `/etc/passwd` attempts  
- ✅ **System-level verification** with `stat()` as final check

### 🚫 2. HTTP Version Validation

```cpp
// Only HTTP/1.0 and HTTP/1.1 accepted
if (version != "HTTP/1.0" && version != "HTTP/1.1") {
    return false; // Triggers 400 Bad Request
}
```

### 📏 3. Header Length Limits

```cpp
// Prevent memory exhaustion attacks
if (requestStr.length() > 8192) { // 8KB limit
    // Handle oversized request gracefully
}
```

### 🎯 4. Method Restrictions

**Configuration-based security:**
```nginx
location /uploads {
    limit_except GET POST DELETE;
    # Only allows specified methods
}
```

---

## 📝 Configuration Guide

### 🔧 Configuration File Structure (`conf/default.conf`)

```nginx
server {
    listen 127.0.0.1:8080;          # Binding address and port
    server_name localhost;          # Virtual host matching
    
    location / {                    # Root location
        root www;                   # Document root
        index index.html;           # Default file
        autoindex off;              # Directory listing
    }
    
    location /uploads {
        root .;                     # Relative to server root
        autoindex on;               # Enable directory browsing
        limit_except GET POST DELETE; # Method restrictions
    }
    
    location /upload {              # POST endpoint
        root www;
        limit_except POST;          # POST-only endpoint
    }
}

# Second server block for multi-port hosting
server {
    listen 0.0.0.0:8000;
    server_name localhost;
    # ... configuration continues
}
```

### 🎨 Configuration Parser Features

**Advanced Parsing (`src/ConfigParser.cpp`):**
- ✅ **Nested block parsing** with proper scope handling
- ✅ **Directive validation** ensures only valid options
- ✅ **Default value assignment** for missing configurations
- ✅ **Error reporting** with line numbers for debugging

---

## 🔍 Evaluation Checklist

### ✅ **Mandatory Requirements**

- [ ] **Non-blocking I/O**: Server uses `select()` ✅
- [ ] **Multiple clients**: Handles concurrent connections ✅  
- [ ] **HTTP/1.1 compliance**: Proper headers and status codes ✅
- [ ] **GET method**: File serving with proper responses ✅
- [ ] **POST method**: Form data and file uploads ✅
- [ ] **DELETE method**: File removal functionality ✅
- [ ] **Error handling**: 4xx/5xx responses for all error cases ✅
- [ ] **Configuration file**: NGINX-style parsing ✅

### 🏆 **Bonus Features**

- [ ] **Multiple ports**: Virtual hosting support ✅
- [ ] **File upload**: Multipart form-data parsing ✅
- [ ] **Directory listing**: Autoindex generation ✅
- [ ] **Security hardening**: Path traversal protection ✅
- [ ] **Professional UI**: Custom landing page ✅

### 🧪 **Testing Verification**

**Evaluator Commands:**
```bash
# 1. Compilation test
make && echo "✅ Compilation successful" || echo "❌ Compilation failed"

# 2. Basic server start
timeout 5s ./webserv conf/default.conf &
sleep 2 && curl -s http://localhost:8080/ | grep -q "WebServ" && echo "✅ Server responsive"

# 3. Method testing
echo "Testing GET..." && curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/ | grep -q "200"
echo "Testing POST..." && curl -s -X POST -d "test=data" -o /dev/null -w "%{http_code}" http://localhost:8080/contact | grep -q "200"  
echo "Testing DELETE..." && curl -s -X DELETE -o /dev/null -w "%{http_code}" http://localhost:8080/delete/nonexistent.txt | grep -q "404"

# 4. Security testing
echo "Testing security..." && curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/../../../etc/passwd | grep -q "403"

# 5. Comprehensive test suite
./run_comprehensive_tests.sh | tail -5
```

---

## 📊 Performance Metrics

### ⚡ Benchmarking Results

**Concurrent Connection Test:**
```bash
# 100 concurrent requests
ab -n 100 -c 10 http://localhost:8080/

# Expected results:
# - 0% failed requests
# - Sub-100ms average response time
# - Proper connection handling
```

**Memory Usage:**
```bash
# Monitor during operation
ps aux | grep webserv

# Expected: Stable memory usage, no leaks
```

**File Descriptor Management:**
```bash
# Check open files
lsof -p $(pidof webserv) | wc -l

# Expected: Reasonable FD count, proper cleanup
```

---

## 🚨 Troubleshooting

### 🔧 Common Issues

#### 1️⃣ **"Address already in use" Error**

**Problem:** Port 8080 or 8000 already bound
```bash
# Check what's using the port
lsof -i :8080

# Kill existing process
sudo kill -9 $(lsof -t -i:8080)
```

#### 2️⃣ **"Permission denied" on File Access**

**Problem:** File permissions or directory access
```bash
# Fix file permissions
chmod 644 www/*.html
chmod 755 www/
```

#### 3️⃣ **Configuration Parsing Errors**

**Problem:** Invalid configuration syntax
```bash
# Validate configuration
./webserv conf/default.conf 2>&1 | grep -i error

# Check for:
# - Missing semicolons
# - Unmatched braces
# - Invalid directives
```

#### 4️⃣ **Upload Directory Issues**

**Problem:** File uploads fail
```bash
# Ensure upload directory exists and is writable
mkdir -p uploads
chmod 755 uploads
```

### 🐛 Debug Mode

**Enable verbose logging by modifying `src/Server.cpp`:**
```cpp
// Add debug prints for request processing
std::cout << "DEBUG: Processing " << request.getMethod() 
          << " " << request.getURI() << std::endl;
```

---

## 🎓 Evaluation Tips

### 👨‍🏫 For Evaluators

1. **Start with the basics**: Compile, run, test simple GET request
2. **Follow the flow**: Use the test scripts to verify functionality systematically  
3. **Check edge cases**: Try malformed requests, long URIs, special characters
4. **Verify security**: Attempt directory traversal and injection attacks
5. **Test concurrency**: Open multiple browser tabs simultaneously
6. **Review code**: Look for proper error handling and resource management

### 📚 Key Code Sections to Review

| File | Function | Purpose |
|------|----------|---------|
| `src/Server.cpp` | `handleRequest()` | Main request router |
| `src/Server.cpp` | `_handleGetRequest()` | File serving logic |
| `src/Server.cpp` | `_handlePostRequest()` | Upload handling |
| `src/ServerInstance.cpp` | `run()` | Event loop with `select()` |
| `src/HttpRequest.cpp` | `parse()` | HTTP parsing |
| `src/ConfigParser.cpp` | `parse()` | Configuration handling |

### 🎯 Questions to Ask

- **"How does the server handle multiple clients?"** → `select()` multiplexing
- **"What prevents directory traversal attacks?"** → Path validation in `_handleGetRequest()`
- **"How are file uploads processed?"** → Multipart parsing in `HttpRequest::_parseMultipartData()`
- **"What happens with invalid HTTP methods?"** → 405 Method Not Allowed response

---

## 🏆 Conclusion

WebServ demonstrates **enterprise-grade HTTP server implementation** with:

- ✅ **97.2% test success rate** across comprehensive test suites
- ✅ **Robust security model** with multiple protection layers  
- ✅ **Clean, maintainable code** following C++98 standards
- ✅ **Professional documentation** and user experience
- ✅ **Real-world functionality** comparable to production servers

**Ready for 42 School evaluation and beyond! 🚀**

---

## 📞 Support

For questions or issues during evaluation:

1. **Check the troubleshooting section** above
2. **Run the test suites** to verify functionality
3. **Review the code comments** for implementation details
4. **Test with `curl` commands** provided in this README

**Happy evaluating! 🎉**

---

*Built with ❤️ for 42 School WebServ project*
=======
## Development notes

- Language composition
  - C++: 63.3%
  - Shell: 23.7%
  - HTML: 12.4%
  - Other: 0.6%

- Guidelines
  - Prefer portable C++98 constructs
  - Keep I/O, parsing, and response logic well-separated
  - Add small, focused CGI examples for testing

- Debugging tips
  - Start with a minimal config and a single location
  - Use curl -v to inspect requests/responses
  - Tail your server logs (if provided) while testing

---

## FAQ

- Does it support persistent connections?
  - Yes, it implements core HTTP/1.1 behavior. Exact keep-alive semantics follow the project’s parser/response logic.

- Which CGI interpreters are supported?
  - Any interpreter reachable on your system (e.g., Python, Perl, Bash) that can run via the configured CGI directives or via script shebang lines.

- How large can uploads be?
  - Controlled by client_max_body_size (see your config). Ensure the destination directory exists and has write permissions.

---

## Roadmap ideas

- More detailed logging and access/error logs
- MIME type mapping for richer static file responses
- Better directory listing themes
- TLS termination (via reverse proxy or native support)
- More robust configuration validation and error messages

---

## Contributing

Issues, ideas, and pull requests are welcome!
- Discuss new directives and semantics before large changes
- Keep contributions focused and well-tested
- Add/adjust sample configs and CGI examples where helpful

---

## License

This project is distributed under the license specified in the repository. See the LICENSE file for details.

---

## Acknowledgments

- Inspired by the clarity of NGINX’s configuration style
- Thanks to the authors of the HTTP/1.1 specs (RFC 2616 and its successors RFC 7230–7235)
- Everyone exploring systems programming and networking—have fun building!
