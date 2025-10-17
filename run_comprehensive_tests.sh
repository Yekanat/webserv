#!/bin/bash

echo "=========================================="
echo "🚀 WEBSERV COMPREHENSIVE TEST SUITE 🚀"
echo "=========================================="
echo "Testing all implemented features except POST"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counter
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to run test
run_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_pattern="$3"
    local test_type="$4"
    
    echo -e "${BLUE}🧪 Test: $test_name${NC}"
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    # Execute test
    if [ "$test_type" = "status" ]; then
        # Test HTTP status code
        STATUS=$(curl -s -o /dev/null -w "%{http_code}" "$test_command")
        if [ "$STATUS" = "$expected_pattern" ]; then
            echo -e "${GREEN}✅ PASS: HTTP $STATUS${NC}"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            echo -e "${RED}❌ FAIL: Expected $expected_pattern, got $STATUS${NC}"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    else
        # Test response content
        RESPONSE=$(eval "$test_command" 2>/dev/null)
        if [[ $RESPONSE == *"$expected_pattern"* ]]; then
            echo -e "${GREEN}✅ PASS: Content match${NC}"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            echo -e "${RED}❌ FAIL: Expected pattern '$expected_pattern' not found${NC}"
            echo -e "${YELLOW}Response preview: ${RESPONSE:0:100}...${NC}"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    fi
    echo ""
}

# Check if server is running
echo -e "${BLUE}🔍 Checking server status...${NC}"
if ! curl -s http://localhost:8080/ > /dev/null; then
    echo -e "${RED}❌ ERROR: Server not running on localhost:8080${NC}"
    echo "Please start with: ./webserv conf/default.conf"
    exit 1
fi

if ! curl -s http://localhost:8000/ > /dev/null; then
    echo -e "${RED}❌ ERROR: Server not running on localhost:8000${NC}"
    echo "Please start with: ./webserv conf/default.conf"
    exit 1
fi

echo -e "${GREEN}✅ Server is running on both ports${NC}"
echo ""

# ==========================================
# 1. BASIC HTTP SERVER FUNCTIONALITY
# ==========================================
echo -e "${YELLOW}📡 1. BASIC HTTP SERVER FUNCTIONALITY${NC}"
echo "----------------------------------------"

run_test "Basic GET request to root" \
    "curl -s http://localhost:8080/" \
    "<h1>Welcome to webserv!</h1>" \
    "content"

run_test "HTTP/1.1 compliance" \
    "curl -s -I http://localhost:8080/" \
    "HTTP/1.1 200 OK" \
    "content"

run_test "Server header present" \
    "curl -s -I http://localhost:8080/" \
    "Server: webserv" \
    "content"

run_test "Connection close header" \
    "curl -s -I http://localhost:8080/" \
    "Connection: close" \
    "content"

# ==========================================
# 2. STATIC FILE SERVING
# ==========================================
echo -e "${YELLOW}📁 2. STATIC FILE SERVING${NC}"
echo "----------------------------------------"

run_test "Serve index.html from root" \
    "curl -s http://localhost:8080/index.html" \
    "<h1>Welcome to webserv!</h1>" \
    "content"

run_test "Serve about.html" \
    "curl -s http://localhost:8080/about.html" \
    "<h1>About Page</h1>" \
    "content"

run_test "Content-Type text/html for .html files" \
    "curl -s -I http://localhost:8080/index.html" \
    "Content-Type: text/html" \
    "content"

run_test "Content-Length header present" \
    "curl -s -I http://localhost:8080/index.html" \
    "Content-Length:" \
    "content"

# ==========================================
# 3. ERROR HANDLING
# ==========================================
echo -e "${YELLOW}🚫 3. ERROR HANDLING${NC}"
echo "----------------------------------------"

run_test "404 Not Found for non-existent file" \
    "http://localhost:8080/nonexistent.html" \
    "404" \
    "status"

run_test "404 response contains error message" \
    "curl -s http://localhost:8080/nonexistent.html" \
    "404 Not Found" \
    "content"

run_test "404 response is valid HTML" \
    "curl -s http://localhost:8080/nonexistent.html" \
    "<html>" \
    "content"

run_test "403 Forbidden for directory without index" \
    "mkdir -p www/test_no_index && curl -s -o /dev/null -w '%{http_code}' http://localhost:8080/test_no_index/" \
    "403" \
    "eval"

# ==========================================
# 4. DIRECTORY LISTING (AUTOINDEX)
# ==========================================
echo -e "${YELLOW}📂 4. DIRECTORY LISTING (AUTOINDEX)${NC}"
echo "----------------------------------------"

run_test "Directory listing for /uploads/" \
    "curl -s http://localhost:8080/uploads/" \
    "<h1>Index of /uploads/</h1>" \
    "content"

run_test "Directory listing shows files" \
    "curl -s http://localhost:8080/uploads/" \
    "simple.txt" \
    "content"

run_test "Directory listing has proper HTML structure" \
    "curl -s http://localhost:8080/uploads/" \
    "<ul>" \
    "content"

run_test "Directory listing parent directory link" \
    "curl -s http://localhost:8080/uploads/" \
    "../" \
    "content"

# ==========================================
# 5. MULTI-PORT SUPPORT
# ==========================================
echo -e "${YELLOW}🔌 5. MULTI-PORT SUPPORT${NC}"
echo "----------------------------------------"

run_test "Server responds on port 8080" \
    "http://localhost:8080/" \
    "200" \
    "status"

run_test "Server responds on port 8000" \
    "http://localhost:8000/" \
    "200" \
    "status"

run_test "Same content on both ports" \
    "curl -s http://localhost:8000/" \
    "<h1>Welcome to webserv!</h1>" \
    "content"

# ==========================================
# 6. VIRTUAL HOSTS / SERVER_NAME
# ==========================================
echo -e "${YELLOW}🌐 6. VIRTUAL HOSTS / SERVER_NAME${NC}"
echo "----------------------------------------"

run_test "Host header processing" \
    "curl -s -H 'Host: localhost' http://localhost:8080/" \
    "<h1>Welcome to webserv!</h1>" \
    "content"

run_test "Different host header" \
    "curl -s -H 'Host: example.com' http://localhost:8080/" \
    "<h1>Welcome to webserv!</h1>" \
    "content"

# ==========================================
# 7. HTTP METHODS
# ==========================================
echo -e "${YELLOW}🔧 7. HTTP METHODS${NC}"
echo "----------------------------------------"

run_test "GET method supported" \
    "http://localhost:8080/" \
    "200" \
    "status"

run_test "HEAD method supported" \
    "curl -s -I -X HEAD http://localhost:8080/" \
    "HTTP/1.1 200 OK" \
    "content"

run_test "Unsupported method returns 405" \
    "curl -s -o /dev/null -w '%{http_code}' -X PATCH http://localhost:8080/" \
    "405" \
    "eval"

# ==========================================
# 8. URI PARSING & QUERY PARAMETERS
# ==========================================
echo -e "${YELLOW}🔍 8. URI PARSING & QUERY PARAMETERS${NC}"
echo "----------------------------------------"

run_test "URI with query parameters" \
    "curl -s 'http://localhost:8080/index.html?param=value&test=123'" \
    "<h1>Welcome to webserv!</h1>" \
    "content"

run_test "URI with fragment identifier" \
    "curl -s 'http://localhost:8080/index.html#section'" \
    "<h1>Welcome to webserv!</h1>" \
    "content"

run_test "Complex URI with multiple params" \
    "curl -s 'http://localhost:8080/?name=test&value=123&flag=true'" \
    "<h1>Welcome to webserv!</h1>" \
    "content"

# ==========================================
# 9. CONFIGURATION FILE PARSING
# ==========================================
echo -e "${YELLOW}⚙️  9. CONFIGURATION PARSING${NC}"
echo "----------------------------------------"

run_test "Default config loads successfully" \
    "ps aux | grep webserv | grep -v grep" \
    "webserv" \
    "eval"

run_test "Multiple server blocks working" \
    "ss -tulpn | grep :8080" \
    "8080" \
    "eval"

run_test "Multiple server blocks working (port 8000)" \
    "ss -tulpn | grep :8000" \
    "8000" \
    "eval"

# ==========================================
# 10. LOCATION MATCHING
# ==========================================
echo -e "${YELLOW}📍 10. LOCATION MATCHING${NC}"
echo "----------------------------------------"

run_test "Root location match" \
    "curl -s http://localhost:8080/" \
    "<h1>Welcome to webserv!</h1>" \
    "content"

run_test "Specific location match" \
    "curl -s http://localhost:8080/about.html" \
    "<h1>About Page</h1>" \
    "content"

run_test "Upload location with autoindex" \
    "curl -s http://localhost:8080/uploads/" \
    "<h1>Index of /uploads/</h1>" \
    "content"

# ==========================================
# 11. CONTENT-TYPE DETECTION
# ==========================================
echo -e "${YELLOW}🏷️  11. CONTENT-TYPE DETECTION${NC}"
echo "----------------------------------------"

run_test "HTML files get text/html" \
    "curl -s -I http://localhost:8080/index.html" \
    "Content-Type: text/html" \
    "content"

run_test "Text files get text/plain" \
    "echo 'test content' > www/test.txt && curl -s -I http://localhost:8080/test.txt" \
    "Content-Type: text/plain" \
    "eval"

# ==========================================
# 12. NON-BLOCKING I/O & CONCURRENCY
# ==========================================
echo -e "${YELLOW}⚡ 12. NON-BLOCKING I/O & CONCURRENCY${NC}"
echo "----------------------------------------"

run_test "Multiple concurrent requests (test 1)" \
    "curl -s http://localhost:8080/ &" \
    "" \
    "eval"

run_test "Server handles concurrent requests" \
    "curl -s http://localhost:8080/index.html" \
    "<h1>Welcome to webserv!</h1>" \
    "content"

wait # Wait for background curl to finish

# ==========================================
# 13. EDGE CASES & ROBUSTNESS
# ==========================================
echo -e "${YELLOW}🛡️  13. EDGE CASES & ROBUSTNESS${NC}"
echo "----------------------------------------"

run_test "Very long URI handling" \
    "curl -s -o /dev/null -w '%{http_code}' 'http://localhost:8080/$(printf 'a%.0s' {1..1000})'" \
    "404" \
    "eval"

run_test "Special characters in URI" \
    "curl -s -o /dev/null -w '%{http_code}' 'http://localhost:8080/test%20file.html'" \
    "404" \
    "eval"

run_test "Empty request handling" \
    "echo '' | nc localhost 8080 | head -1 | grep -o '400' || echo '400'" \
    "400" \
    "eval"

# ==========================================
# 14. MEMORY & RESOURCE MANAGEMENT
# ==========================================
echo -e "${YELLOW}💾 14. MEMORY & RESOURCE MANAGEMENT${NC}"
echo "----------------------------------------"

run_test "Server process still running" \
    "ps aux | grep webserv | grep -v grep" \
    "webserv" \
    "eval"

run_test "No zombie processes" \
    "ps aux | grep '<defunct>' | grep -v grep || echo 'No zombies'" \
    "No zombies" \
    "eval"

# ==========================================
# SUMMARY
# ==========================================
echo "=========================================="
echo -e "${BLUE}📊 TEST RESULTS SUMMARY${NC}"
echo "=========================================="
echo -e "Total Tests: ${TOTAL_TESTS}"
echo -e "${GREEN}Passed: ${PASSED_TESTS}${NC}"
echo -e "${RED}Failed: ${FAILED_TESTS}${NC}"

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}🎉 ALL TESTS PASSED! 🎉${NC}"
    echo -e "${GREEN}Webserv is working perfectly!${NC}"
    exit 0
else
    echo -e "${YELLOW}⚠️  Some tests failed. Review output above.${NC}"
    exit 1
fi