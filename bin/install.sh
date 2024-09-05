#!/bin/bash

# Check the OS type and handle non-Linux systems
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    if pidof systemd > /dev/null; then
        echo "==========================================================
        🚀 Systemd Detected! Proceeding with the installation... 🚀
        =========================================================="
    else
        echo "
        ==========================================================
        ⚠️  Unsupported Linux Distribution  ⚠️
        ==========================================================
        ❌ Systemd is not detected on this Linux distribution.
        This install script only supports Linux distributions
        with systemd for service management.

        Please switch to a Linux distribution with systemd support
        and try again. 🐧
        ==========================================================
        "
        exit 1
    fi
elif [[ "$OSTYPE" == "darwin"* ]]; then
    echo "
    ==========================================================
    🍎 Unsupported Operating System: macOS 🍎
    ==========================================================
    ❌ This install script is not compatible with macOS.
    It is designed to run only on Linux distributions with
    systemd for service management.

    Please switch to a Linux system with systemd support. 🐧

    OR... this script can be easily extended by a developer
    with ample need to support their OS, in which case, I
    invite you to become a collaborator and help make
    Forti-hole a more robust Fortigate management tool! 🚀
    ==========================================================
    "
    exit 1
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
    echo "
    ==========================================================
    🪟 Unsupported Operating System: Windows 🪟
    ==========================================================
    ❌ This install script is not compatible with Windows.
    It is designed to run only on Linux distributions with
    systemd for service management.

    Please switch to a Linux system with systemd support. 🐧

    OR... this script can be easily extended by a developer
    with ample need to support their OS, in which case, I
    invite you to become a collaborator and help make
    Forti-hole a more robust Fortigate management tool! 🚀
    ==========================================================
    "
    exit 1
else
    echo "
    ==========================================================
    ❓ Unsupported Operating System: $OSTYPE ❓
    ==========================================================
    ❌ This install script only supports Linux distributions
    with systemd for service management.

    Please switch to a Linux system with systemd support. 🐧

    OR... this script can be easily extended by a developer
    with ample need to support their OS, in which case, I
    invite you to become a collaborator and help make
    Forti-hole a more robust Fortigate management tool! 🚀
    ==========================================================
    "
    exit 1
fi

SERVICE_FILE="./install/forti-hole.service"
TIMER_FILE="./install/forti-hole-5am.timer"
SYSTEMD_PATH="/etc/systemd/system"
RUN_SCRIPT_PATH="$(pwd)/bin/run.sh"
WORKING_DIRECTORY="$(pwd)"

current_user=$(whoami)
current_group=$(id -gn)

# Update ExecStart in the service file
echo "Updating ExecStart line in $SERVICE_FILE with the correct path..."
sed -i "s|ExecStart=.*|ExecStart=$RUN_SCRIPT_PATH|g" "$SERVICE_FILE" || { echo "Failed to update $SERVICE_FILE"; exit 1; }

# Copy service and timer files to systemd system directory
echo "Copying $SERVICE_FILE to $SYSTEMD_PATH..."
sudo cp "$SERVICE_FILE" "$SYSTEMD_PATH" || { echo "Failed to copy $SERVICE_FILE"; exit 1; }

echo "Copying $TIMER_FILE to $SYSTEMD_PATH..."
sudo cp "$TIMER_FILE" "$SYSTEMD_PATH" || { echo "Failed to copy $TIMER_FILE"; exit 1; }

# Update ExecStart in the service file
echo "Updating ExecStart line in $SERVICE_FILE with the correct path..."
sed -i "s|ExecStart=.*|ExecStart=$RUN_SCRIPT_PATH|g" "$SERVICE_FILE" || { echo "Failed to update ExecStart in $SERVICE_FILE"; exit 1; }

# Update User in the service file
echo "Updating User line in $SERVICE_FILE with the current user..."
sed -i "s|User=.*|User=$current_user|g" "$SERVICE_FILE" || { echo "Failed to update User in $SERVICE_FILE"; exit 1; }

# Update Group in the service file
echo "Updating Group line in $SERVICE_FILE with the current group..."
sed -i "s|Group=.*|Group=$current_group|g" "$SERVICE_FILE" || { echo "Failed to update Group in $SERVICE_FILE"; exit 1; }

# Update WorkingDirectory in the service file
echo "Updating WorkingDirectory line in $SERVICE_FILE with the current working directory..."
sed -i "s|WorkingDirectory=.*|WorkingDirectory=$WORKING_DIRECTORY|g" "$SERVICE_FILE" || { echo "Failed to update WorkingDirectory in $SERVICE_FILE"; exit 1; }

echo "Service file updated successfully."

# Reload systemd to recognize the new service and timer
echo "Reloading systemd daemon..."
sudo systemctl daemon-reload || { echo "Failed to reload systemd"; exit 1; }

# Enable and start the timer
echo "Enabling and starting forti-hole-5am.timer..."
sudo systemctl enable forti-hole-5am.timer || { echo "Failed to enable forti-hole-5am.timer"; exit 1; }
sudo systemctl start forti-hole-5am.timer || { echo "Failed to start forti-hole-5am.timer"; exit 1; }

# Enable and start the service
echo "Enabling and starting forti-hole service..."
sudo systemctl enable forti-hole.service || { echo "Failed to enable forti-hole service"; exit 1; }
sudo systemctl start forti-hole.service || { echo "Failed to start forti-hole service"; exit 1; }

echo "Installation and service setup completed successfully."
