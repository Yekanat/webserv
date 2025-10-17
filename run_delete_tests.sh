#!/bin/bash

echo "🗑️  WEBSERV DELETE METHOD TESTS"
echo "==============================="

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PASS_COUNT=0
TOTAL_COUNT=0

# Function to run test
run_test() {
    local test_name="$1"
    local expected="$2"
    local command="$3"
    
    echo -e "${BLUE}🧪 Test: $test_name${NC}"
    TOTAL_COUNT=$((TOTAL_COUNT + 1))
    
    # Execute test
    RESULT=$(eval "$command" 2>/dev/null)
    
    if echo "$RESULT" | grep -q "$expected"; then
        echo -e "${GREEN}✅ PASS${NC}"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo -e "${RED}❌ FAIL${NC}"
        echo "Expected: $expected"
        echo "Got: $RESULT"
    fi
    echo ""
}

echo -e "${YELLOW}🔍 Checking server status...${NC}"
curl -s http://localhost:8080/ > /dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Server not running on port 8080${NC}"
    exit 1
fi
echo -e "${GREEN}✅ Server is running${NC}"
echo ""

echo -e "${BLUE}📁 Preparing test files...${NC}"
# Create test files in allowed directories
echo "DELETE test file" > www/delete/delete_test.txt
echo "DELETE upload test" > uploads/delete_upload_test.txt
echo "DELETE protected file" > www/protected_delete.txt
echo ""

echo -e "${BLUE}🗑️  1. DELETE METHOD FUNCTIONALITY${NC}"
echo "----------------------------------------"

# Test 1: DELETE existing file in /delete directory (should work)
run_test "DELETE existing file in /delete" "200 OK" "curl -s -i -X DELETE http://localhost:8080/delete/delete_test.txt | head -1"

# Test 2: DELETE non-existent file (should return 404)
run_test "DELETE non-existent file" "404 Not Found" "curl -s -i -X DELETE http://localhost:8080/delete/nonexistent.txt | head -1"

# Test 3: DELETE in uploads directory (should work)
run_test "DELETE file in uploads directory" "200 OK" "curl -s -i -X DELETE http://localhost:8080/uploads/delete_upload_test.txt | head -1"

# Test 4: DELETE in root directory (should be forbidden)
run_test "DELETE in root directory (forbidden)" "403 Forbidden" "curl -s -i -X DELETE http://localhost:8080/protected_delete.txt | head -1"

# Test 5: DELETE directory (should fail)
run_test "DELETE directory (should fail)" "404 Not Found" "curl -s -i -X DELETE http://localhost:8080/delete/ | head -1"

echo -e "${BLUE}🛡️  2. SECURITY & VALIDATION${NC}"
echo "----------------------------------------"

# Test 6: DELETE with directory traversal attempt
run_test "DELETE with directory traversal" "403 Forbidden" "curl -s -i -X DELETE 'http://localhost:8080/delete/../protected_delete.txt' | head -1"

# Test 7: DELETE with absolute path attempt
run_test "DELETE with absolute path" "403 Forbidden" "curl -s -i -X DELETE 'http://localhost:8080/delete//etc/passwd' | head -1"

# Test 8: DELETE with null byte injection
run_test "DELETE with null byte" "400 Bad Request" "curl -s -i -X DELETE 'http://localhost:8080/delete/test%00.txt' | head -1"

echo -e "${BLUE}📋 3. HTTP COMPLIANCE${NC}"
echo "----------------------------------------"

# Test 9: DELETE response headers
echo -e "${BLUE}🧪 Test: DELETE response has proper headers${NC}"
TOTAL_COUNT=$((TOTAL_COUNT + 1))
HEADERS=$(curl -s -i -X DELETE http://localhost:8080/delete/nonexistent.txt | head -10)
if echo "$HEADERS" | grep -q "Content-Type: text/html" && echo "$HEADERS" | grep -q "Server: webserv"; then
    echo -e "${GREEN}✅ PASS${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}❌ FAIL - Missing proper headers${NC}"
fi
echo ""

# Test 10: DELETE response body format
echo -e "${BLUE}🧪 Test: DELETE response has HTML body${NC}"
TOTAL_COUNT=$((TOTAL_COUNT + 1))
BODY=$(curl -s -X DELETE http://localhost:8080/delete/nonexistent.txt)
if echo "$BODY" | grep -q "<html>" && echo "$BODY" | grep -q "DELETE"; then
    echo -e "${GREEN}✅ PASS${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}❌ FAIL - Invalid HTML response${NC}"
fi
echo ""

echo -e "${BLUE}🧹 Cleaning up test files...${NC}"
rm -f www/protected_delete.txt
echo ""

echo "==============================="
echo -e "${BLUE}📊 DELETE TEST RESULTS${NC}"
echo "==============================="
echo -e "Total Tests: $TOTAL_COUNT"
echo -e "Passed: ${GREEN}$PASS_COUNT${NC}"
echo -e "Failed: ${RED}$((TOTAL_COUNT - PASS_COUNT))${NC}"
echo ""

if [ $PASS_COUNT -eq $TOTAL_COUNT ]; then
    echo -e "${GREEN}🎉 ALL DELETE TESTS PASSED! 🎉${NC}"
    echo -e "${GREEN}DELETE method is working perfectly!${NC}"
else
    echo -e "${RED}⚠️  Some DELETE tests failed${NC}"
    echo -e "${YELLOW}Check the implementation${NC}"
fi
echo ""