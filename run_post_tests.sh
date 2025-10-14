#!/bin/bash

echo "=== POST METHOD TEST SUITE ==="
echo "Starting webserv tests..."

# Controlla che il server sia running
if ! curl -s http://localhost:8080/ > /dev/null; then
    echo "ERROR: Server not running on localhost:8080"
    echo "Start with: ./webserv conf/default.conf"
    exit 1
fi

echo "✅ Server is running"

# Test 1: Form data semplice
echo "🧪 Test 1: Simple form data"
RESPONSE=$(curl -s -X POST -d "name=TestUser&email=test@example.com" http://localhost:8080/contact)
if [[ $RESPONSE == *"TestUser"* && $RESPONSE == *"test@example.com"* ]]; then
    echo "✅ PASS: Form data parsing"
else
    echo "❌ FAIL: Form data parsing"
    echo "Response: $RESPONSE"
fi

# Test 2: File upload
echo "🧪 Test 2: File upload"
echo "test content" > /tmp/test_upload.txt
RESPONSE=$(curl -s -X POST -F "file=@/tmp/test_upload.txt" http://localhost:8080/upload)
if [[ $RESPONSE == *"test_upload.txt"* && $RESPONSE == *"uploads/"* ]]; then
    echo "✅ PASS: File upload"
else
    echo "❌ FAIL: File upload"
    echo "Response: $RESPONSE"
fi

# Test 3: Verifica file salvato
echo "🧪 Test 3: Verify saved file"
if ls uploads/*test_upload.txt > /dev/null 2>&1; then
    SAVED_CONTENT=$(cat uploads/*test_upload.txt)
    if [[ $SAVED_CONTENT == "test content" ]]; then
        echo "✅ PASS: File content preserved"
    else
        echo "❌ FAIL: File content corrupted"
        echo "Expected: 'test content', Got: '$SAVED_CONTENT'"
    fi
else
    echo "❌ FAIL: File not saved"
    echo "Files in uploads/:"
    ls -la uploads/ 2>/dev/null || echo "uploads/ directory doesn't exist"
fi

# Test 4: URL encoding
echo "🧪 Test 4: URL encoding"
RESPONSE=$(curl -s -X POST -d "name=John%20Doe&message=Hello%20%26%20World" http://localhost:8080/contact)
if [[ $RESPONSE == *"John Doe"* && $RESPONSE == *"Hello & World"* ]]; then
    echo "✅ PASS: URL decoding"
else
    echo "❌ FAIL: URL decoding"
    echo "Response: $RESPONSE"
fi

# Test 5: Empty POST
echo "🧪 Test 5: Empty POST"
RESPONSE=$(curl -s -X POST -H "Content-Length: 0" http://localhost:8080/submit)
if [[ $RESPONSE == *"No data received"* ]]; then
    echo "✅ PASS: Empty POST handling"
else
    echo "❌ FAIL: Empty POST handling"
    echo "Response: $RESPONSE"
fi

echo "=== TEST SUITE COMPLETED ==="