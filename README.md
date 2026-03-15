# Kinova Leaf Grasping System
An autonomous robotic system for collecting plant samples through computer vision.

![Docker](https://img.shields.io/badge/docker-%230db7ed.svg?style=for-the-badge&logo=docker&logoColor=white)
![Visual Studio Code](https://img.shields.io/badge/Visual%20Studio%20Code-0078d7.svg?style=for-the-badge&logo=visual-studio-code&logoColor=white)
![ROS 2 Humble](https://img.shields.io/badge/ROS%202-Humble-22314E.svg?style=for-the-badge&logo=ros&logoColor=white)

[![python](https://img.shields.io/badge/Python-3.10.12-3776AB.svg?style=flat&logo=python&logoColor=white)](https://www.python.org)

## Overview
This system uses a Kinova Gen3 6-DOF robotic arm with a Robotiq 2F-85 gripper to autonomously identify and collect plant leaf samples using computer vision. The robot integrates ROS 2 with MoveIt for motion planning and carries an on-arm camera for real-time plant detection.

## Setup & Getting Started

### Build Container (Optional)
To create the Docker image locally:
```bash
make build-image
```
*Note: Building takes significant time. Consider pulling from the registry directly using `make bash` instead.*

### Initialize Docker Network
Create a Docker network to connect your local machine, VNC display, and the Kinova arm:
```bash
make network
```

### Start VNC Display Server
Launch the VNC container to visualize the simulation and control interface via your browser:
```bash
make vnc
```
Then open `localhost:8080/vnc.html` in your browser to access the display.

## Development Environment

### Option 1: Dev Container (Recommended)
If using VSCode, open the project in the included Dev Container (`.devcontainer/`). This provides the full ROS 2 environment with all dependencies.

### Option 2: Docker Shell
Start an interactive Docker environment:
```bash
make bash
```

### Build the ROS 2 Project
Once in the container, build all packages:
```bash
colcon build
```

## Running the System

### Simulation Mode
Launch the Gazebo simulator with MoveIt motion planning:
```bash
make moveit
```
This starts the simulated Kinova arm and the MoveIt motion planning interface.

### Example: Moving the Arm
In a separate terminal (use `docker exec` to open a new shell in the running container), test arm movement:
```bash
make moveit-example
```

### Controlling the Arm Directly
Use the simple arm controller for basic motion commands:
```bash
make moveit-sac
```

## Computer Vision

### Camera Setup
The Kinova Gen3 arm includes an on-board camera for real-time plant detection and leaf identification. The camera data is available through ROS 2 topics.

### Using Real Camera Data
For plant detection algorithm development, real sensor data is recommended:
1. **Capture data with `rosbag`**: Record camera streams and depth data from the physical arm
2. **Replay locally**: Playback the recorded data in your development environment without needing hardware
3. **Test offline**: Develop and test computer vision algorithms on real plant samples

### Recommended Workflow
- Start by testing with your local webcam
- Develop detection/segmentation algorithms
- Test with recorded rosbag data from the actual camera
- Deploy to the physical system once validated

### Vision Topics
- `/camera/color/image_raw` - RGB image stream
- `/camera/depth/image_rect_raw` - Depth image stream
- Image processing utilities are available via cv_bridge integration

## Real Hardware Deployment

To control the physical Kinova arm (requires proper network configuration):
```bash
make moveit-target
```
Ensure your network is properly configured to communicate with the arm's controller.

## Project Structure

- **`src/kinova_python_control/`** - ROS 2 Python package for arm control and MoveIt integration
- **`src/next_best_view/`** - Core package for computing and executing next best view actions during plant sample collection
- **`src/kinova_gen3_6dof_robotiq_2f_85_moveit_config/`** - MoveIt configuration files for the Kinova Gen3 arm with Robotiq gripper
- **`Dockerfile`** - Container definition with ROS 2 Humble, MoveIt, and all dependencies
- **`Makefile`** - Convenient commands for building, deploying, and running the system

## Dependencies

- ROS 2 Humble
- MoveIt (motion planning and control)
- Gazebo (simulation)
- OpenCV (via cv_bridge for image processing)
- Kinova kortex drivers and vision modules

All dependencies are pre-configured in the Docker container.