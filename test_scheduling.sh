#!/bin/bash

echo "Testing priority scheduling fixes..."
echo "Expected behavior:"
echo "1. P1 (x50) should start first (highest priority)"
echo "2. P2 (x70) should start second" 
echo "3. P3 (x40) should start third"
echo "4. P4 (x60) and P5 (x80) should be queued as READY"
echo "5. When first 3 finish, P4 and P5 should automatically start"
echo ""

# Start the shell in background
./shell &
SHELL_PID=$!

# Give shell time to start
sleep 1

# Send commands to test priority scheduling
echo "run ./prog x40 40 P3" | nc -w 1 localhost 1234 2>/dev/null || echo "run ./prog x40 40 P3"
sleep 1
echo "run ./prog x50 50 P1" | nc -w 1 localhost 1234 2>/dev/null || echo "run ./prog x50 50 P1"  
sleep 1
echo "run ./prog x60 60 P4" | nc -w 1 localhost 1234 2>/dev/null || echo "run ./prog x60 60 P4"
sleep 1
echo "run ./prog x70 70 P2" | nc -w 1 localhost 1234 2>/dev/null || echo "run ./prog x70 70 P2"
sleep 1
echo "run ./prog x80 80 P5" | nc -w 1 localhost 1234 2>/dev/null || echo "run ./prog x80 80 P5"
sleep 1

echo "list" | nc -w 1 localhost 1234 2>/dev/null || echo "list"
sleep 2

echo "exit" | nc -w 1 localhost 1234 2>/dev/null || echo "exit"

# Clean up
kill $SHELL_PID 2>/dev/null
