#!/bin/bash

#test_file.sh runs the shell with fixed sequence of commands

./shell << 'EOF'
echo hello
echo "hello world"
echo 'single quote'
echo      testing leading/trailing whitespace    ...
history
procread version
procread "version"
procread
procread a b
procread /version
procread fakefile
"ls" /
"echo" hello world
history
exit
EOF

echo "Showing contents of .421history"
echo "-------------------------------"
cat .421history
