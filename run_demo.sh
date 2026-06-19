#!/bin/bash
# Demo script for Plant Monitoring System
# This script sets up a virtual serial connection and runs the demo

set -e

echo "========================================"
echo "Plant Monitor Demo Setup"
echo "========================================"
echo ""

# Check if socat is installed
if ! command -v socat &> /dev/null; then
    echo "ERROR: socat is not installed."
    echo "Please install it first:"
    echo "  macOS:   brew install socat"
    echo "  Ubuntu:  sudo apt-get install socat"
    echo "  Fedora:  sudo dnf install socat"
    exit 1
fi

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "Build directory not found. Running cmake..."
    mkdir -p build
    cd build
    cmake ..
    make
    cd ..
fi

# Check if executable exists
if [ ! -f "build/plant_monitor" ]; then
    echo "Executable not found. Building..."
    cd build
    make
    cd ..
fi

echo "Starting virtual serial port with socat..."
echo "(This will stay running in the background)"
echo ""

# Create a temporary file to store the PTY paths
PTY_FILE=$(mktemp)

# Start socat in the background and capture the PTY paths
socat -d -d pty,raw,echo=0,link=/tmp/plant_serial_write pty,raw,echo=0,link=/tmp/plant_serial_read 2>&1 | grep -o '/dev/pts/[0-9]*' | head -2 > "$PTY_FILE" &
SOCAT_PID=$!

# Give socat time to create the devices
sleep 2

if [ ! -s "$PTY_FILE" ]; then
    echo "Warning: Could not detect PTY paths automatically."
    echo "Using symlinks at /tmp/plant_serial_write and /tmp/plant_serial_read"
    WRITE_PTY="/tmp/plant_serial_write"
    READ_PTY="/tmp/plant_serial_read"
else
    WRITE_PTY=$(sed -n '1p' "$PTY_FILE")
    READ_PTY=$(sed -n '2p' "$PTY_FILE")
fi

echo "Virtual serial ports created:"
echo "  Write to (mock Arduino): $WRITE_PTY or /tmp/plant_serial_write"
echo "  Read from (application):  $READ_PTY or /tmp/plant_serial_read"
echo ""

# Update config.hpp to use the correct serial port
CONFIG_FILE="include/config.hpp"
if grep -q "/dev/ttyACM0" "$CONFIG_FILE"; then
    echo "Note: Update config.hpp to use /tmp/plant_serial_read for the serial port"
    echo "      Currently set to: /dev/ttyACM0"
    echo ""
fi

echo "========================================"
echo "Starting Mock Arduino Sensor..."
echo "========================================"
python3 mock_arduino.py "$WRITE_PTY" &
MOCK_PID=$!

echo ""
echo "========================================"
echo "Demo is running!"
echo "========================================"
echo ""
echo "To run the plant monitor application, in another terminal:"
echo "  cd $(pwd)"
echo "  ./build/plant_monitor"
echo ""
echo "Or if you've updated config.hpp to /tmp/plant_serial_read:"
echo "  cd build && ./plant_monitor"
echo ""
echo "To view the web interface:"
echo "  Open http://localhost:8080 in your browser"
echo ""
echo "Press Ctrl+C to stop all processes and clean up"
echo ""

# Set up cleanup on exit
cleanup() {
    echo ""
    echo "Cleaning up..."
    kill $MOCK_PID 2>/dev/null || true
    kill $SOCAT_PID 2>/dev/null || true
    rm -f "$PTY_FILE"
    rm -f /tmp/plant_serial_write /tmp/plant_serial_read
    echo "Demo stopped."
}

trap cleanup EXIT INT TERM

# Wait for user interrupt
wait $MOCK_PID
