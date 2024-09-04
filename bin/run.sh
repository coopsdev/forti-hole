#!/bin/bash

CONFIG_FILE="./config.yaml"
ENV_FILE="./.env"
EXECUTABLE="./build/meson/forti-hole"

any_missing=false

# Check if config.yaml exists
if [[ ! -f "$CONFIG_FILE" ]]; then
    echo "DEBUG: $CONFIG_FILE is missing."
    any_missing=true
fi

# Check if the executable exists
if [[ ! -f "$EXECUTABLE" ]]; then
    echo "DEBUG: $EXECUTABLE is missing."
    any_missing=true
fi

# If any required files are missing, run the build script and exit if necessary
if [[ "$any_missing" == true ]]; then
    echo "DEBUG: Running build.sh to resolve missing dependencies..."

    # Run the build script
    ./bin/build.sh

    # Print final messages and exit if config.yaml or .env were missing
    if [[ ! -f "$CONFIG_FILE" || ! -f "$ENV_FILE" ]]; then
        echo "ERROR: config.yaml or .env were missing. They have been created in the build directory."
        echo "Please configure them to match your Fortigate device before running the program."
        exit 1
    fi
fi

# If all checks passed, run the program
echo "Starting forti-hole..."
$EXECUTABLE || { echo "ERROR: Execution failed"; exit 1; }
