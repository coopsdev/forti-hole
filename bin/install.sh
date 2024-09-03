#!/bin/bash

# Define paths
SERVICE_FILE="../install/forti-hole.service"
TIMER_FILE="../install/forti-hole-5am.timer"
SYSTEMD_PATH="/etc/systemd/system"
RUN_SCRIPT_PATH="$(pwd)/run.sh"

# Update ExecStart in the service file
echo "Updating ExecStart line in $SERVICE_FILE with the correct path..."
sed -i "s|ExecStart=.*|ExecStart=$RUN_SCRIPT_PATH|g" "$SERVICE_FILE" || { echo "Failed to update $SERVICE_FILE"; exit 1; }

# Copy service and timer files to systemd system directory
echo "Copying $SERVICE_FILE to $SYSTEMD_PATH..."
sudo cp "$SERVICE_FILE" "$SYSTEMD_PATH" || { echo "Failed to copy $SERVICE_FILE"; exit 1; }

echo "Copying $TIMER_FILE to $SYSTEMD_PATH..."
sudo cp "$TIMER_FILE" "$SYSTEMD_PATH" || { echo "Failed to copy $TIMER_FILE"; exit 1; }

# Reload systemd to recognize the new service and timer
echo "Reloading systemd daemon..."
sudo systemctl daemon-reload || { echo "Failed to reload systemd"; exit 1; }

# Enable and start the service
echo "Enabling and starting forti-hole service..."
sudo systemctl enable forti-hole.service || { echo "Failed to enable forti-hole service"; exit 1; }
sudo systemctl start forti-hole.service || { echo "Failed to start forti-hole service"; exit 1; }

# Enable and start the timer
echo "Enabling and starting forti-hole-5am.timer..."
sudo systemctl enable forti-hole-5am.timer || { echo "Failed to enable forti-hole-5am.timer"; exit 1; }
sudo systemctl start forti-hole-5am.timer || { echo "Failed to start forti-hole-5am.timer"; exit 1; }

echo "Installation and service setup completed successfully."
