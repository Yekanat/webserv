#!/bin/bash

# Crea directory di test
mkdir -p test_files
cd test_files

# File di testo semplice
echo "Hello World Test File" > simple.txt

# File HTML
cat > test.html << 'EOF'
<!DOCTYPE html>
<html><head><title>Test</title></head>
<body><h1>Test HTML File</h1></body></html>
EOF

# File binario piccolo (immagine fake)
echo -e "\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x00\x01\x00\x00\x00\x01" > tiny.png

# File più grande per test size limit
head -c 1024 /dev/urandom > large.bin

cd ..