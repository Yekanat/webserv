#!/bin/bash

echo "⚡ PERFORMANCE & STRESS TESTS"
echo "=============================="

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Test concurrent requests
echo -e "${BLUE}🚀 Testing concurrent requests...${NC}"

# Function to make request and measure time
test_concurrent() {
    local num_requests=$1
    echo -e "${YELLOW}Testing $num_requests concurrent requests...${NC}"
    
    start_time=$(date +%s.%N)
    
    # Launch concurrent requests
    for i in $(seq 1 $num_requests); do
        curl -s http://localhost:8080/ > /dev/null &
    done
    
    # Wait for all to complete
    wait
    
    end_time=$(date +%s.%N)
    duration=$(echo "$end_time - $start_time" | bc -l)
    
    echo -e "${GREEN}✅ $num_requests requests completed in ${duration}s${NC}"
    echo ""
}

# Test different levels of concurrency
test_concurrent 5
test_concurrent 10
test_concurrent 20

# Test rapid sequential requests
echo -e "${BLUE}🔄 Testing rapid sequential requests...${NC}"
start_time=$(date +%s.%N)

for i in {1..50}; do
    curl -s http://localhost:8080/ > /dev/null
done

end_time=$(date +%s.%N)
duration=$(echo "$end_time - $start_time" | bc -l)
echo -e "${GREEN}✅ 50 sequential requests completed in ${duration}s${NC}"
echo ""

# Test large file serving
echo -e "${BLUE}📦 Testing large file serving...${NC}"
start_time=$(date +%s.%N)

RESPONSE=$(curl -s http://localhost:8080/large.html)
RESPONSE_SIZE=${#RESPONSE}

end_time=$(date +%s.%N)
duration=$(echo "$end_time - $start_time" | bc -l)

echo -e "${GREEN}✅ Large file (${RESPONSE_SIZE} bytes) served in ${duration}s${NC}"
echo ""

# Test server resource usage
echo -e "${BLUE}💾 Server resource usage:${NC}"
ps aux | grep webserv | grep -v grep | awk '{print "CPU: " $3 "%, Memory: " $4 "%, PID: " $2}'
echo ""

# Test file descriptor usage
echo -e "${BLUE}📁 File descriptor usage:${NC}"
WEBSERV_PID=$(pgrep webserv)
if [ ! -z "$WEBSERV_PID" ]; then
    FD_COUNT=$(ls /proc/$WEBSERV_PID/fd 2>/dev/null | wc -l)
    echo -e "${GREEN}Open file descriptors: $FD_COUNT${NC}"
else
    echo -e "${RED}Webserv process not found${NC}"
fi
echo ""

echo -e "${GREEN}🎯 Performance tests completed!${NC}"