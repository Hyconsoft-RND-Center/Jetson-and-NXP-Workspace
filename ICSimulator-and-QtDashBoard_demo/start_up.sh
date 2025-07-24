# #!/bin/bash

# # Load CAN modules
# sudo modprobe can
# sudo modprobe vcan

# # If vcan0 already exists, skip creation
# if ip link show vcan0 &>/dev/null; then
#     echo "vcan0 already exists, skipping creation"
# else
#     sudo ip link add dev vcan0 type vcan
#     if [ $? -ne 0 ]; then
#         echo "Failed to create vcan0 interface"
#         exit 1
#     fi
# fi

# # Bring up the vcan0 interface
# sudo ip link set up vcan0
# if [ $? -ne 0 ]; then
#     echo "Failed to bring up vcan0 interface"
#     exit 1
# fi

# echo "vcan0 interface successfully set up"
