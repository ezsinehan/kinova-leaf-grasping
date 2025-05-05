# IOT4AG Hackathon Manipulator Challenge
![Docker](https://img.shields.io/badge/docker-%230db7ed.svg?style=for-the-badge&logo=docker&logoColor=white)
![Visual Studio Code](https://img.shields.io/badge/Visual%20Studio%20Code-0078d7.svg?style=for-the-badge&logo=visual-studio-code&logoColor=white)

[![github](https://img.shields.io/badge/GitHub-ucmercedrobotics-181717.svg?style=flat&logo=github)](https://github.com/ucmercedrobotics)
[![website](https://img.shields.io/badge/Website-UCMRobotics-5087B2.svg?style=flat&logo=telegram)](https://robotics.ucmerced.edu/)
[![python](https://img.shields.io/badge/Python-3.10.12-3776AB.svg?style=flat&logo=python&logoColor=white)](https://www.python.org)
[![pre-commits](https://img.shields.io/badge/pre--commit-enabled-brightgreen?logo=pre-commit&logoColor=white)](https://github.com/pre-commit/pre-commit)

<!-- [![Checked with mypy](http://www.mypy-lang.org/static/mypy_badge.svg)](http://mypy-lang.org/) -->
<!-- TODO: work to enable pydocstyle -->
<!-- [![pydocstyle](https://img.shields.io/badge/pydocstyle-enabled-AD4CD3)](http://www.pydocstyle.org/en/stable/) -->

<!-- [![arXiv](https://img.shields.io/badge/arXiv-2409.04653-b31b1b.svg)](https://arxiv.org/abs/2409.04653) -->

## How to Start
[Optional] Build your container:
NOTE: we recommend pulling directly from the registry using `make bash` (see below) instead of building from scratch. It's a huge container and it's easier to just pull.
However, if you wish to build because you're making changes to the container, then feel free to build yourself.
```bash
make build-image
```

You'll be forwarding graphic display to noVNC using your browser.
First start the Docker network that will manage these packets:
To start, make your local Docker network to connect your VNC client, local machine, and Kinova together. You'll use this later when remote controlling the Kinova.
```bash
make network
```

Next, standup the VNC container to forward X11 to your web browser. You can see this by searching `localhost:8080/vnc.html` in your browser.
```bash
make vnc
```

## Simulation
To start the Docker environment:
```bash
make bash
```
Note, if you use VSCode, we have a Dev Container for you to work in. This is much simpler if you're familiar with it.

If you're not using the Dev Container, make sure the ROS2 project is built. To be safe, run the following from within the container:
```bash
root@f30dcdb836cc:/iot4ag-challenge# colcon build
```

Finally, to launch the ROS2 simulated drivers for MoveIt Kortex control:
```bash
make moveit
```

## Example
If you want to see the robot move in sim or on target, you can launch the custom example node we prebuilt in a separate shell alongside make moveit. Running `docker exec` 
in your host machine's terminal will open a new Docker shell, from there this command will just move the arm to an arbitrary position:
```bash
make moveit-example
```

## Vision
If you're wanting to do computer vision with your project, come see me about getting real data from the camera. 
We will use `rosbag` to capture real world data from the camera on the arm and you can replay it on your own computer within your work environment.
From experience, I recommend working with your personal laptop camera to test out algorithms before moving to target hardware. 