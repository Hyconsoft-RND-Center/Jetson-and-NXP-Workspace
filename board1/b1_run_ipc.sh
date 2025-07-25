#!/bin/bash

echo "=== IPC LAUNCH SCRIPT BOARD1 STARTED ===" >> /tmp/ipc-log.txt
date >> /tmp/ipc-log.txt

unset ROS_LOCALHOST_ONLY
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_DOMAIN_ID=13
export CROSS_COMPILE=/usr/bin/aarch64-linux-gnu-
source /opt/ros/humble/setup.bash
source /home/workspace/autoware_auto_msgs/install/setup.bash


echo "Waiting 30 seconds before starting binary..." >> /tmp/ipc-log.txt
sleep 30

# Navigate to the directory
cd /home/workspace/git/sample_user/build/autoware_subscriber/ || {
  echo "Failed to change directory. Exiting."
  exit 1
} 

./ipc-shm-sample_uio
