#!/usr/bin/env python3

"""
Test keypoints sender for turnToPerson module
Sends simulated person keypoints to test the module functionality
"""

import yarp
import time
import math
import sys

def create_test_keypoints(frame_num):
    """
    Create test keypoints data that simulates a person moving across the image
    """
    # Simulate a person moving from left to right
    base_u = 200 + (frame_num % 100) * 2  # Move from u=200 to u=400
    base_v = 240  # Center vertically
    
    # Create keypoints for a typical person pose
    keypoints = [
        ("nose", base_u, base_v - 50),
        ("neck", base_u, base_v - 30),
        ("left_shoulder", base_u - 30, base_v - 20),
        ("right_shoulder", base_u + 30, base_v - 20),
        ("left_elbow", base_u - 50, base_v + 10),
        ("right_elbow", base_u + 50, base_v + 10),
        ("left_wrist", base_u - 60, base_v + 40),
        ("right_wrist", base_u + 60, base_v + 40),
        ("left_hip", base_u - 20, base_v + 50),
        ("right_hip", base_u + 20, base_v + 50),
        ("left_knee", base_u - 25, base_v + 100),
        ("right_knee", base_u + 25, base_v + 100),
        ("left_ankle", base_u - 30, base_v + 150),
        ("right_ankle", base_u + 30, base_v + 150)
    ]
    
    return keypoints

def main():
    # Initialize YARP
    yarp.Network.init()
    
    if not yarp.Network.checkNetwork():
        print("ERROR: YARP server is not running")
        return 1
    
    # Create output port
    port = yarp.BufferedPortBottle()
    if not port.open("/keypointSender/out"):
        print("ERROR: Could not open output port")
        return 1
    
    print("Keypoints sender started. Connecting to /turnToPerson/keypoints:i...")
    
    # Try to connect to the turnToPerson module
    connected = False
    for i in range(10):  # Try for 10 seconds
        if yarp.Network.connect("/keypointSender/out", "/turnToPerson/keypoints:i"):
            connected = True
            print("✓ Connected to turnToPerson module")
            break
        time.sleep(1)
        print(f"Waiting for connection... ({i+1}/10)")
    
    if not connected:
        print("WARNING: Could not connect to turnToPerson module, but will send data anyway")
    
    print("Sending keypoints data... (Press Ctrl+C to stop)")
    
    frame_num = 0
    try:
        while True:
            # Create test keypoints
            keypoints = create_test_keypoints(frame_num)
            
            # Create YARP bottle
            bottle = port.prepare()
            bottle.clear()
            
            external_bottle = bottle.addList()
            person_bottle = external_bottle.addList()
            for name, u, v in keypoints:
                # Add each keypoint as a sub-bottle
                keypoint_bottle = person_bottle.addList()
                keypoint_bottle.addString(name)
                keypoint_bottle.addFloat64(u)
                keypoint_bottle.addFloat64(v)

            port.write()
            
            print(f"Frame {frame_num}: Sent {len(keypoints)} keypoints (centroid ~{keypoints[0][1]:.0f}, {keypoints[0][2]:.0f})")
            
            frame_num += 1
            time.sleep(0.1)  # 10 Hz
            
    except KeyboardInterrupt:
        print("\nStopping keypoints sender...")
    
    # Clean up
    port.close()
    yarp.Network.fini()
    return 0

if __name__ == "__main__":
    sys.exit(main())
