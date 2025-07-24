#!/bin/bash

# Set default port
PORT=5000

# Start listening for UDP messages on localhost and given port
echo "Listening for UDP packets on port $PORT..."
nc -ul 127.0.0.1 $PORT
