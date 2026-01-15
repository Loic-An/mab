# MAB

## Overview

This project aims to control the height of cylinders mounted on DC motors,
monitored by Time-of-Flight (ToF) sensors arranged in a matrix array. The system
uses depth data from an Xbox 360 Kinect sensor to dynamically adjust cylinder
positions based on captured depth information.

## Getting Started

### Prerequisites

This project has some hardware requirements:

- Raspberry Pi (4B used here)*
- Xbox Kinect V1 (for 360)
- lots of small DC motor N20 (we will use 16 motors, 12V, 1000rpm)
- lots of motor driver ic (L293 used here, 1 for 2 motors)
- lots of ToF sensor (1 for each motor)
- TCA9548A boards to have ToF sensors on the same I2C bus
- PCA9685 boards to control the DC motors speed and direction

*In reality you only need a board that can run Linux, is I2C capable and that
have a least a USB port, we choose a Pi4B because we had one in our hands.

### Installation (build from source)

First, open a terminal and clone our repo:

```bash
git clone https://github.com/Loic-An/mab.git
```

Then go to the project root folder:

```bash
cd mab
```

You'll need some dependencies/third_partiy libraries(mentionned in the credits
section), execute this to install them:

```bash
sudo apt install make # if not already installed
make deps
```

Finally, you can compile the project:

```bash
make
```

You'll see a file created called 'mab', if not something went wrong.

### Usage

```bash
./mab
```

## Project Structure

```
mab/            # Project root folder
├── inc/        # Where the header files are
│   └── *.h     # 
├── src/        # Where the source code is
│   └── *.c     #
├── LICENSE     # 
├── mab         # Executable present if you compile the project
├── Makefile    #
└── README.md   # You are here
```

# Credits

- [ST API](https://www.st.com/en/embedded-software/stsw-img005.html) to
  communicate with the ToF sensors VL53L0X
- [libfreenect](https://github.com/OpenKinect/libfreenect) to communicate with
  the Kinect
