#!/bin/bash

# Function to set up a Python virtual environment
setup_venv() {
    python3 -m venv venv || { echo "Failed to create virtual environment"; exit 1; }
    source venv/bin/activate || { echo "Failed to activate virtual environment"; exit 1; }

    # Trap the script exit to deactivate the virtual environment
    trap deactivate_venv EXIT

    # Upgrade pip to the latest version
    pip install --upgrade pip || { echo "Failed to upgrade pip"; exit 1; }
}

# Function to deactivate the virtual environment
deactivate_venv() {
    deactivate || { echo "Failed to deactivate virtual environment"; }
}

# Function to install Conan and Meson inside the venv
install_conan_meson() {
    pip install conan meson || { echo "Failed to install Conan and Meson"; exit 1; }
}

# Function to check if a command exists
command_exists() {
    command -v "$1" &> /dev/null
}

# Ensure Python is installed (especially on Windows)
check_python_installed() {
    if ! command_exists python && ! command_exists python3; then
        echo "**********************************************"
        echo "*                                            *"
        echo "*            🚨 Python Not Detected 🚨         *"
        echo "*                                            *"
        echo "*   Python is not installed! Please install   *"
        echo "*   Python before proceeding.                 *"
        echo "*                                            *"
        echo "*   Download it from: https://python.org      *"
        echo "*                                            *"
        echo "*   After installation, ensure 'python' or    *"
        echo "*   'python3' is in your PATH.                *"
        echo "*                                            *"
        echo "**********************************************"
        exit 1
    fi
}

# Linux-specific setup for venv
setup_linux_venv() {
    # Check if python3-venv is installed on Linux
    if ! dpkg -s python3-venv &> /dev/null; then
        echo "python3-venv is not installed, installing..."
        sudo apt-get update
        sudo apt-get install -y python3-venv || { echo "Failed to install python3-venv"; exit 1; }
    fi
    setup_venv
}

# Windows-specific setup for venv
setup_windows_venv() {
    setup_venv
}

# macOS-specific setup for venv
setup_macos_venv() {
    setup_venv
}

# Main script logic
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    check_python_installed
    setup_linux_venv
    install_conan_meson
elif [[ "$OSTYPE" == "msys" ]]; then
    check_python_installed
    setup_windows_venv
    install_conan_meson
elif [[ "$OSTYPE" == "darwin"* ]]; then
    check_python_installed
    setup_macos_venv
    install_conan_meson
else
    echo "Unsupported operating system: $OSTYPE"
    exit 1
fi

# Check for 'release' Conan profile
if ! conan profile show release &> /dev/null; then
    echo "Creating 'release' Conan profile..."
    conan profile detect --name=release --force || { echo "Failed to create Conan profile"; exit 1; }

    echo "Configuring 'release' profile..."

    PROFILE_PATH=$(conan profile path release)

    if [[ "$OSTYPE" == "darwin"* ]]; then
        sed -i '' 's|compiler=.*|compiler=apple-clang|' "$PROFILE_PATH"
        sed -i '' 's|compiler.libcxx=.*|compiler.libcxx=libc++|' "$PROFILE_PATH"
        sed -i '' 's|os=.*|os=Macos|' "$PROFILE_PATH"
        sw_version=$(sw_vers -productVersion)
        sed -i '' "s|os.version=.*|os.version=$sw_version|" "$PROFILE_PATH"
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        sed -i 's|compiler=.*|compiler=gcc|' "$PROFILE_PATH"
        sed -i 's|compiler.libcxx=.*|compiler.libcxx=libstdc++|' "$PROFILE_PATH"
        sed -i 's|os=.*|os=Linux|' "$PROFILE_PATH"
    elif [[ "$OSTYPE" == "msys" ]]; then
        sed -i 's|compiler=.*|compiler=Visual Studio|' "$PROFILE_PATH"
        sed -i 's|compiler.version=.*|compiler.version=17|' "$PROFILE_PATH"
        sed -i 's|compiler.runtime=.*|compiler.runtime=MD|' "$PROFILE_PATH"
        sed -i 's|arch=.*|arch=x86_64|' "$PROFILE_PATH"
        sed -i 's|os=.*|os=Windows|' "$PROFILE_PATH"
    fi

    sed -i 's|build_type=.*|build_type=Release|' "$PROFILE_PATH"
    sed -i 's|compiler.cppstd=.*|compiler.cppstd=23|' "$PROFILE_PATH"
fi

# Install dependencies using Conan
conan install . --build=missing || { echo "Conan install failed"; exit 1; }

# Build the project with the 'release' profile
if [[ "$OSTYPE" == "msys" ]]; then
    meson setup builddir --backend=vs2017 || { echo "Meson setup failed"; exit 1; }
else
    meson setup builddir || { echo "Meson setup failed"; exit 1; }
fi

conan build . -pr:a release --build=missing || { echo "Build failed"; exit 1; }

# Check for existing config.yaml and .env files
if [[ ! -f ./config.yaml ]]; then
    cp ./config.example.yaml ./config.yaml || { echo "Failed to copy config.yaml"; exit 1; }
    echo "config.yaml created in build directory. Please configure it to match your Fortigate device before running the program."
fi

if [[ ! -f ./.env ]]; then
    cp ./.env.example ./.env || { echo "Failed to copy .env"; exit 1; }
    echo ".env created in build directory. Please configure it to match your Fortigate device before running the program."
fi

echo "Build and installation completed successfully."
