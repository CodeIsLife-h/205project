#!/bin/bash

# Test script for Unix Process Manager
# Run this script to test all functionality

echo "=== Unix Process Manager Test Script ==="
echo

# Compile both programs
echo "1. Compiling programs..."
gcc -o manager shell.c
gcc -o prog prog.c

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
else
    echo "✗ Compilation failed"
    exit 1
fi

echo

# Test prog individually
echo "2. Testing prog individually..."
./prog test_prog.txt 3
if [ -f test_prog.txt ]; then
    echo "✓ prog created file successfully"
    echo "File contents:"
    cat test_prog.txt
    echo
else
    echo "✗ prog failed to create file"
fi

echo

# Test manager with basic commands
echo "3. Testing manager with basic commands..."
echo "Starting manager in background..."

# Create a test input file
cat > test_input.txt << EOF
run ./prog test1.txt 10 P1
run ./prog test2.txt 15 P2
run ./prog test3.txt 10 P3
run ./prog test4.txt 8 P4
run ./prog test5.txt 12 P5
run ./prog test6.txt 20 P6
run ./prog test7.txt 18 P7
run ./prog test8.txt 25 P8
run ./prog test9.txt 14 P9
list
stop 1000
list
resume 1000
list
kill 1001
list
exit
EOF

echo "Test input created. Run the following commands manually:"
echo
echo "Commands to test:"
echo "1. ./manager"
echo "2. run ./prog test1.txt 3 P1"
echo "3. run ./prog test2.txt 3 P2"
echo "4. run ./prog test3.txt 3 P3"
echo "5. run ./prog test4.txt 3 P4"
echo "6. list"
echo "7. stop [PID_from_list]"
echo "8. list"
echo "9. resume [PID_from_list]"
echo "10. list"
echo "11. kill [PID_from_list]"
echo "12. list"
echo "13. exit"
echo

echo "Expected behavior:"
echo "- Only 3 processes should be running at any time"
echo "- P1 should have highest priority (lowest number)"
echo -"- stop should suspend a process and start the next highest priority"
echo "- resume should put a stopped process back to ready/running"
echo "- kill should terminate a process"
echo "- Files should be created with progress messages"
echo

echo "After testing, check created files:"
echo "ls -la *.txt"
echo "cat test1.txt"
echo "cat test2.txt"
echo

echo "=== Test Script Complete ==="

