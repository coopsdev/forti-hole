#!/bin/bash

CONFIG_FILE="./config.yaml"
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

    # Check if config.yaml is missing and provide guidance for next steps
    if [[ ! -f "$CONFIG_FILE" ]]; then
        echo "
            ==========================================================

            ✅ config.yaml has been created in the build directory.

            🛠️  Scroll up and follow the instructions above to configure Forti-hole.

            ==========================================================
        "
    fi
fi

# If all checks passed, run the program
echo "Starting forti-hole..."
$EXECUTABLE || { echo "ERROR: Execution failed"; exit 1; }
