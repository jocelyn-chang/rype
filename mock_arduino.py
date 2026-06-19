#!/usr/bin/env python3
"""
Mock Arduino Sensor Simulator

This script simulates an Arduino sending soil moisture sensor readings
over a serial connection. It creates a virtual serial port and sends
realistic sensor data to demonstrate the plant monitoring system.

Usage:
    python3 mock_arduino.py [serial_port]
    
    If no serial_port is provided, it creates a virtual port using socat.
"""

import sys
import time
import random
import os

def generate_sensor_readings():
    """
    Generate a realistic sequence of sensor readings that demonstrates
    the full functionality of the plant monitoring system.
    
    Yields: (adc_value, description) tuples
    """
    # Phase 1: Healthy plant in configured ideal range
    # ADC values: 300-360
    print("=== Phase 1: Starting with healthy soil ===", file=sys.stderr)
    for i in range(8):
        yield (random.randint(300, 360), f"Healthy reading {i+1}/8")
    
    # Phase 2: Gradual drying toward upper ideal bound
    print("=== Phase 2: Soil gradually drying ===", file=sys.stderr)
    for i in range(6):
        value = 340 + (i * 10) + random.randint(-8, 8)
        yield (value, f"Drying {i+1}/6")
    
    # Phase 3: Getting dry (crossing above RAW 380 threshold)
    # Need 3 consecutive dry readings to trigger alert
    print("=== Phase 3: Soil becoming dry (should trigger alert) ===", file=sys.stderr)
    for i in range(5):
        value = random.randint(390, 460)
        yield (value, f"Dry reading {i+1}/5")
    
    # Phase 4: Critical dry
    print("=== Phase 4: Very dry soil ===", file=sys.stderr)
    for i in range(4):
        yield (random.randint(500, 580), f"Very dry {i+1}/4")
    
    # Phase 5: Watering event (sudden recovery)
    print("=== Phase 5: Plant watered - recovery ===", file=sys.stderr)
    for i in range(3):
        value = 420 - (i * 45) + random.randint(-12, 12)
        yield (value, f"Recovery {i+1}/3")
    
    # Phase 6: Return to healthy state
    print("=== Phase 6: Back to healthy ===", file=sys.stderr)
    for i in range(8):
        yield (random.randint(300, 360), f"Healthy again {i+1}/8")

def send_sensor_data(port_path):
    """
    Send mock sensor data to the specified serial port.
    
    Args:
        port_path: Path to the serial port (e.g., '/dev/ttyACM0' or 'pts/X')
    """
    print(f"Opening serial port: {port_path}", file=sys.stderr)
    
    try:
        with open(port_path, 'w') as port:
            print(f"Connected! Sending sensor data...", file=sys.stderr)
            print(f"(Press Ctrl+C to stop)", file=sys.stderr)
            
            for adc_value, description in generate_sensor_readings():
                message = f"RAW:{adc_value}\n"
                port.write(message)
                port.flush()
                
                # Calculate approximate moisture percentage for display
                moisture = ((adc_value - 590) / (273 - 590)) * 100
                moisture = max(0, min(100, moisture))
                
                print(f"Sent: {message.strip()} [{description}] (~{moisture:.1f}% moisture)", 
                      file=sys.stderr)
                time.sleep(2)  # Send a reading every 2 seconds
            
            print("\n=== Demo sequence complete! Repeating... ===\n", file=sys.stderr)
            # Keep sending healthy readings indefinitely
            while True:
                adc_value = random.randint(300, 360)
                message = f"RAW:{adc_value}\n"
                port.write(message)
                port.flush()
                print(f"Sent: {message.strip()} (healthy)", file=sys.stderr)
                time.sleep(2)
                
    except KeyboardInterrupt:
        print("\n\nStopped by user.", file=sys.stderr)
    except Exception as e:
        print(f"\nError: {e}", file=sys.stderr)
        sys.exit(1)

def main():
    if len(sys.argv) > 1:
        # Use provided serial port
        port_path = sys.argv[1]
        send_sensor_data(port_path)
    else:
        print("=" * 70, file=sys.stderr)
        print("MOCK ARDUINO SENSOR SIMULATOR", file=sys.stderr)
        print("=" * 70, file=sys.stderr)
        print("\nThis script simulates Arduino sensor readings.", file=sys.stderr)
        print("\nTo use with socat virtual serial port:", file=sys.stderr)
        print("  1. In terminal 1: socat -d -d pty,raw,echo=0 pty,raw,echo=0", file=sys.stderr)
        print("  2. Note the /dev/pts/X paths created", file=sys.stderr)
        print("  3. In terminal 2: python3 mock_arduino.py /dev/pts/X", file=sys.stderr)
        print("  4. In terminal 3: ./plant_monitor (update config.hpp first)", file=sys.stderr)
        print("\nAlternatively, use the run_demo.sh script for automatic setup!", file=sys.stderr)
        print("=" * 70, file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
