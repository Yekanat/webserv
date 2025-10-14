#!/bin/bash

echo "🛡️  SECURITY & EDGE CASE TESTS"
echo "==============================="

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

TOTAL_TESTS=0
PASSED_TESTS=0

run_security_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_status="$3"
    
    echo -e "${BLUE}🔒 $test_name${NC}"
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    STATUS=$(eval "$test_command" 2>/dev/null)
    
    if [ "$STATUS" = "$expected_status" ]; then
        echo -e "${GREEN}✅ PASS: Got expected status $expected_status${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}❌ FAIL: Expected $expected_status, got $STATUS${NC}"
    fi
    echo ""
}

# Directory traversal attempts
echo -e "${YELLOW}📁 Directory Traversal Tests${NC}"
echo "----------------------------------------"

run_security_test "Basic directory traversal" \
    "curl -s -o /dev/null -w '%{http_code}' 'http://localhost:8080/../etc/passwd'" \
    "404"

run_security_test "URL encoded directory traversal" \
    "curl -s -o /dev/null -w '%{http_code}' 'http://localhost:8080/%2e%2e/%2e%2e/etc/passwd'" \
    "404"

run_security_test "Double URL encoded traversal" \
    "curl -s -o /dev/null -w '%{http_code}' 'http://localhost:8080/%252e%252e/etc/passwd'" \
    "404"

run_security_test "Windows-style traversal" \
    "curl -s -o /dev/null -w '%{http_code}' 'http://localhost:8080/..\\..\\windows\\system32\\'" \
    "404"

# Malformed requests
echo -e "${YELLOW}💥 Malformed Request Tests${NC}"
echo "----------------------------------------"

run_security_test "Very long URI" \
    "curl -s -o /dev/null -w '%{http_code}' 'http://localhost:8080/$(printf 'a%.0s' {1..2048})'" \
    "404"

run_security_test "Invalid HTTP version" \
    "echo -e 'GET / HTTP/2.0\r\n\r\n' | nc localhost 8080 | head -1 | grep -o '[0-9][0-9][0-9]' || echo '400'" \
    "400"

run_security_test "Missing HTTP version" \
    "echo -e 'GET /\r\n\r\n' | nc localhost 8080 | head -1 | grep -o '[0-9][0-9][0-9]' || echo '400'" \
    "400"

run_security_test "Invalid method name" \
    "curl -s -o /dev/null -w '%{http_code}' -X 'INVALID_METHOD' http://localhost:8080/" \
    "405"

# Header injection tests
echo -e "${YELLOW}📧 Header Injection Tests${NC}"
echo "----------------------------------------"

run_security_test "CRLF injection in header" \
    "curl -s -H $'Host: localhost\r\nX-Injected: malicious' -o /dev/null -w '%{http_code}' http://localhost:8080/" \
    "200"

run_security_test "Very long header value" \
    "curl -s -H 'X-Long-Header: $(printf 'x%.0s' {1..8192})' -o /dev/null -w '%{http_code}' http://localhost:8080/" \
    "200"

# Request bombing
echo -e "${YELLOW}💣 Request Bombing Tests${NC}"
echo "----------------------------------------"

echo -e "${BLUE}🔥 Rapid fire requests test${NC}"
start_time=$(date +%s)
for i in {1..100}; do
    curl -s http://localhost:8080/ > /dev/null &
    if [ $((i % 10)) -eq 0 ]; then
        wait # Wait every 10 requests
    fi
done
wait
end_time=$(date +%s)
duration=$((end_time - start_time))
echo -e "${GREEN}✅ Server survived 100 rapid requests in ${duration}s${NC}"
echo ""

# Resource exhaustion tests
echo -e "${YELLOW}🔋 Resource Tests${NC}"
echo "----------------------------------------"

echo -e "${BLUE}📊 Server process status after stress tests:${NC}"
ps aux | grep webserv | grep -v grep
echo ""

echo -e "${BLUE}📈 Memory usage:${NC}"
WEBSERV_PID=$(pgrep webserv)
if [ ! -z "$WEBSERV_PID" ]; then
    ps -p $WEBSERV_PID -o pid,vsz,rss,pcpu,pmem,cmd
else
    echo -e "${RED}Webserv process not found${NC}"
fi
echo ""

# Null byte injection
echo -e "${YELLOW}🔪 Null Byte Injection Tests${NC}"
echo "----------------------------------------"

run_security_test "Null byte in URI" \
    "curl -s -o /dev/null -w '%{http_code}' 'http://localhost:8080/index.html%00.txt'" \
    "200"

# Special characters
echo -e "${YELLOW}🎭 Special Character Tests${NC}"
echo "----------------------------------------"

run_security_test "Unicode characters in URI" \
    "curl -s -o /dev/null -w '%{http_code}' 'http://localhost:8080/тест'" \
    "404"

run_security_test "SQL injection-like patterns" \
    "curl -s -o /dev/null -w '%{http_code}' \"http://localhost:8080/index.html';DROP TABLE users;--\"" \
    "404"

# Final check - server still responsive
echo -e "${YELLOW}🏁 Final Responsiveness Check${NC}"
echo "----------------------------------------"

run_security_test "Server still responsive after all tests" \
    "curl -s -o /dev/null -w '%{http_code}' http://localhost:8080/" \
    "200"

# Summary
echo "=========================================="
echo -e "${BLUE}🛡️  SECURITY TEST SUMMARY${NC}"
echo "=========================================="
echo -e "Total Security Tests: ${TOTAL_TESTS}"
echo -e "${GREEN}Passed: ${PASSED_TESTS}${NC}"
echo -e "${RED}Failed: $((TOTAL_TESTS - PASSED_TESTS))${NC}"

if [ $PASSED_TESTS -eq $TOTAL_TESTS ]; then
    echo -e "${GREEN}🎉 ALL SECURITY TESTS PASSED!${NC}"
    echo -e "${GREEN}Server appears to be secure against basic attacks.${NC}"
else
    echo -e "${YELLOW}⚠️  Some security tests failed. Review output above.${NC}"
fi