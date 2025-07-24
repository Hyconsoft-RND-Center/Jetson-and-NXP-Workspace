# ICSimulator-and-QtDashBoard

## Dependencies
It will be tried to use minimum number of dependencies as much as possible. The current dependencies are as follows:
- ubuntu 22.04 jammy jellyfish
- ROS2 Humble packages installed
- Cmake mimimum version: 3.14
- Compiler:
    - GCC C/C++ Compiler (x86-64 Linux GNU): 11.2.0; or
    - Clang C/C++ Compiler (x86-64 PC Linux GNU): 14.0.0

## CAN Interface
```bash
$ cd ICSimulator-and-QtDashBoard
$ chmod +x start_up.sh
$ ./start_up.sh
```
-Or

```bash
$ sudo ip link set can0 up type can bitrate 500000
```
![vcan0](https://github.com/user-attachments/assets/e8cdaef0-be82-4381-a615-b852e3f359b1)

## Installing QT5 Creator and packages
```bash
$ sudo apt-get install qt5-default qtcreator build-essential qml-module-qtquick2 qml-module-qtquick-extras qml-module-qtquick-controls2
```

## Architecture
-Use Case 1

-Use Case 2
![IC-Simulator](https://github.com/user-attachments/assets/bfa07290-94ed-49ea-af42-cc30c9b68d9d)

## Git Clone
- git clone:
```bash
$ git clone git@github.com:Hyconsoft-RND-Center/ICSimulator-and-QtDashBoard.git
```

## Compiling
```bash
$ cd ICSimulator-and-QtDashBoard
$ cmake -DCMAKE_BUILD_TYPE:STRING=Release -S . -B build && cd build && make
```

## Run
- Terminal 1
```bash
$ cd build/ICSimulator
$ ./ICSimulator
```
- Terminal 2
```bash
$ cd build/Dashboard
$ ./dashboard
```

## Documentation

## Contribution