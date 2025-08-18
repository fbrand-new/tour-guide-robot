# Turn To Person Module

This YARP module receives person keypoints as input and turns the robot towards the detected person using baseControl2.

## Features

- Receives person keypoints through a YARP port
- Calculates the centroid of the person from valid keypoints
- Computes angular velocity to turn the robot towards the person
- Sends velocity commands directly to baseControl2 via YARP ports
- Falls back to navigation interface if available
- Provides RPC interface for runtime configuration and control

## Input Format

The module expects keypoints data in the following format on the input port:
```
(keypoint_name u_image v_image) (keypoint_name u_image v_image) ...
```

Where:
- `keypoint_name`: Name of the keypoint (e.g., "nose", "left_eye", "right_shoulder")
- `u_image`: U coordinate in the image (pixels)
- `v_image`: V coordinate in the image (pixels)

Only keypoints with coordinates > 0 are considered valid.

## Usage

### Basic Usage
```bash
turnToPerson --from turnToPerson.ini
```

### With custom configuration
```bash
turnToPerson --name myTurnToPerson --angular_gain 0.15 --max_angular_vel 45.0
```

## Ports

### Input Ports
- `/turnToPerson/keypoints:i`: Receives person keypoints data

### Output/Service Ports
- `/turnToPerson/rpc`: RPC server for module control
- `/turnToPerson/velocity:o`: Velocity commands output to baseControl2

## RPC Commands

The module provides the following RPC commands:

- `start`: Start person tracking
- `stop`: Stop person tracking and stop robot
- `get status`: Get current module status (active/inactive)
- `set angular_gain <value>`: Set angular gain for control
- `set max_angular_vel <value>`: Set maximum angular velocity (deg/s)
- `set dead_zone <value>`: Set dead zone size (pixels)
- `help`: Show available commands
- `quit`: Stop the module

## Configuration Parameters

- `name`: Module name (default: "turnToPerson")
- `period`: Module update period in seconds (default: 0.1)
- `image_center_u`: Image center U coordinate (default: 320.0)
- `image_center_v`: Image center V coordinate (default: 240.0)
- `angular_gain`: Proportional gain for angular velocity (default: 0.1)
- `max_angular_vel`: Maximum angular velocity in deg/s (default: 30.0)
- `dead_zone`: Dead zone around image center in pixels (default: 20.0)
- `command_timeout`: Timeout for velocity commands in seconds (default: 1.0)
- `navigation_server`: Navigation server port (default: "/navigation2D_nws_yarp")
- `localization_server`: Localization server port (default: "/localization2D_nws_yarp")
- `map_server`: Map server port (default: "/map2D_nws_yarp")
- `basecontrol_port`: BaseControl2 input port (default: "/baseControl/input/command/data:i")

## Dependencies

- YARP
- baseControl2 module running
- Navigation infrastructure (optional, direct port communication is used)

## Control Logic

1. The module receives keypoints from the input port
2. It calculates the centroid of all valid keypoints
3. It computes the error between the person centroid and image center
4. If the error is outside the dead zone, it calculates proportional angular velocity
5. It sends velocity commands directly to baseControl2 via YARP bottle messages
6. The robot turns until the person is centered in the image

## Command Format

The module sends velocity commands to baseControl2 in the format:
```
(3 x_speed y_speed angular_speed pwm_gain)
```
Where:
- `3`: Command type (cartesian speed)
- `x_speed`: Forward/backward velocity (always 0 for this module)
- `y_speed`: Left/right velocity (always 0 for this module)  
- `angular_speed`: Rotation velocity in deg/s
- `pwm_gain`: Power gain (always 100)

## Example Connection

To connect a keypoint detector to this module:
```bash
yarp connect /keypointDetector/output:o /turnToPerson/keypoints:i
```

To control the module via RPC:
```bash
yarp rpc /turnToPerson/rpc
```
