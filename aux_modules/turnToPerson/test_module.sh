#!/bin/bash

# Test script for turnToPerson module

echo "=== Turn To Person Module Test ==="
echo

# Check if YARP server is running
if ! yarp where > /dev/null 2>&1; then
    echo "ERROR: YARP server is not running. Please start it with 'yarp server'"
    exit 1
fi

echo "✓ YARP server is running"

# Start the module in background
echo "Starting turnToPerson module..."
turnToPerson --from turnToPerson.ini &
MODULE_PID=$!

# Wait a bit for the module to start
sleep 2

# Check if module is running
if ! ps -p $MODULE_PID > /dev/null; then
    echo "ERROR: Module failed to start"
    exit 1
fi

echo "✓ Module started successfully (PID: $MODULE_PID)"

# Test RPC commands
echo
echo "Testing RPC commands..."

# Test status
echo -n "Testing 'get status'... "
STATUS=$(echo "get status" | yarp rpc /turnToPerson/rpc)
echo "Response: $STATUS"

# Test help
echo -n "Testing 'help'... "
HELP=$(echo "help" | yarp rpc /turnToPerson/rpc)
echo "Response received (length: ${#HELP})"

# Test parameter setting
echo -n "Testing 'set angular_gain 0.2'... "
SET_GAIN=$(echo "set angular_gain 0.2" | yarp rpc /turnToPerson/rpc)
echo "Response: $SET_GAIN"

# Send test keypoints data
echo
echo "Sending test keypoints data..."

# Create a test keypoints bottle and send it
# Format: (keypoint_name u_image v_image) (keypoint_name u_image v_image) ...
TEST_DATA='((nose 300.0 200.0) (left_eye 280.0 180.0) (right_eye 320.0 180.0))'

echo "Sending: $TEST_DATA"
echo "$TEST_DATA" | yarp write ... /turnToPerson/keypoints:i &
SENDER_PID=$!

sleep 1
kill $SENDER_PID 2>/dev/null

echo
echo "Test completed. Stopping module..."

# Stop the module
echo "stop" | yarp rpc /turnToPerson/rpc
sleep 1

# Force kill if still running
if ps -p $MODULE_PID > /dev/null; then
    kill $MODULE_PID 2>/dev/null
    sleep 1
fi

echo "✓ Module stopped"
echo
echo "=== Test completed ==="
