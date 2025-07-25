#!/bin/bash

echo "=== IPC LAUNCH SCRIPT STARTED ===" >> /tmp/ipc-log.txt
date >> /tmp/ipc-log.txt

echo "Waiting 30 seconds before starting binary..." >> /tmp/ipc-log.txt
sleep 30

cd /home/workspace/git/sample_user/build || {
  echo "Failed to cd into build directory" >> /tmp/ipc-log.txt
  exit 1
}

chmod +x ipc-shm-sample_uio
echo "Running binary with input..." >> /tmp/ipc-log.txt

# Send "11" as input to the binary and capture output
echo "3" | ./ipc-shm-sample_uio >> /tmp/ipc-log.txt 2>&1

echo "=== IPC LAUNCH SCRIPT ENDED ===" >> /tmp/ipc-log.txt
