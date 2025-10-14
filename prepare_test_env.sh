#!/bin/bash

echo "🔧 PREPARING TEST ENVIRONMENT"
echo "=============================="

# Create test files in www directory
echo "Creating test files..."

# Simple text file
echo "This is a plain text file for testing content-type detection." > www/test.txt

# CSS file
cat > www/style.css << 'EOF'
body {
    font-family: Arial, sans-serif;
    margin: 40px;
    background-color: #f5f5f5;
}
h1 {
    color: #333;
}
EOF

# JavaScript file
cat > www/script.js << 'EOF'
function testFunction() {
    console.log("This is a test JavaScript file");
    return "Hello from JavaScript!";
}
EOF

# Large HTML file for testing
cat > www/large.html << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>Large Test File</title>
    <style>
        body { font-family: Arial, sans-serif; }
        .content { padding: 20px; }
    </style>
</head>
<body>
    <div class="content">
        <h1>Large Test File</h1>
        <p>This is a larger HTML file for testing purposes.</p>
EOF

# Add some repetitive content to make it larger
for i in {1..50}; do
    echo "        <p>This is paragraph number $i. Lorem ipsum dolor sit amet, consectetur adipiscing elit.</p>" >> www/large.html
done

echo "    </div>
</body>
</html>" >> www/large.html

# Create directory structure for testing
mkdir -p www/subdir
echo "<h1>Subdirectory File</h1>" > www/subdir/index.html
echo "Subdirectory text file" > www/subdir/file.txt

mkdir -p www/empty_dir

# Create test directory without index
mkdir -p www/no_index_dir
echo "File in no index dir" > www/no_index_dir/some_file.txt

echo "✅ Test environment prepared!"
echo ""
echo "Created files:"
echo "- www/test.txt (text file)"
echo "- www/style.css (CSS file)"  
echo "- www/script.js (JavaScript file)"
echo "- www/large.html (large HTML file)"
echo "- www/subdir/ (subdirectory with files)"
echo "- www/empty_dir/ (empty directory)"
echo "- www/no_index_dir/ (directory without index)"
echo ""