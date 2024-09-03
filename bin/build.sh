#!/bin/bash

# Check if running on Windows and ensure Python is installed
if [[ "$OSTYPE" == "msys" ]]; then
    if ! command -v python &> /dev/null && ! command -v python3 &> /dev/null; then
        echo "**********************************************"
        echo "*                                            *"
        echo "*            🚨 Windows Detected 🚨            *"
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
fi

# Ensure Conan is installed
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    if ! command vconan &> /dev/null; then
        echo "Conan could not be found, installing..."
        sudo apt install conan || { echo "Conan installation failed"; exit 1; }
    fi
elif [[ "$OSTYPE" == "darwin"* ]]; then
    if ! command -v conan &> /dev/null; then
        echo "Conan could not be found, installing..."
        brew install conan || { echo "Conan installation failed"; exit 1; }
    fi
elif [[ "$OSTYPE" == "msys" ]]; then
    if ! command -v conan &> /dev/null; then
        echo "Conan could not be found, installing..."
        python -m pip install conan || { echo "Conan installation failed"; exit 1; }
    fi
fi

# Ensure Meson is installed
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    if ! command -v meson &> /dev/null; then
        echo "Meson could not be found, installing..."
        sudo apt install meson || { echo "Meson installation failed"; exit 1; }
    fi
elif [[ "$OSTYPE" == "darwin"* ]]; then
    if ! command -v meson &> /dev/null; then
        echo "Meson could not be found, installing..."
        brew install meson || { echo "Meson installation failed"; exit 1; }
    fi
elif [[ "$OSTYPE" == "msys" ]]; then
    if ! command -v meson &> /dev/null; then
        echo "Meson could not be found, installing..."
        python -m pip install meson || { echo "Meson installation failed"; exit 1; }
    fi
fi

# Check for 'release' Conan profile
if ! conan profile show release &> /dev/null; then
    echo "Creating 'release' Conan profile..."
    conan profile new release --detect || { echo "Failed to create Conan profile"; exit 1; }

    echo "Configuring 'release' profile..."
    conan profile update settings.build_type=Release release
    conan profile update settings.compiler.cppstd=23 release

    if [[ "$OSTYPE" == "darwin"* ]]; then
        conan profile update settings.compiler=clang release
        conan profile update settings.compiler.libcxx=libc++ release
        conan profile update settings.os=Macos release
        conan profile update settings.os.version=$(sw_vers -productVersion) release
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        conan profile update settings.compiler=gcc release
        conan profile update settings.compiler.libcxx=libstdc++ release
        conan profile update settings.os=Linux release
    elif [[ "$OSTYPE" == "msys" ]]; then
        conan profile update settings.compiler="Visual Studio" release
        conan profile update settings.compiler.version=17 release
        conan profile update settings.compiler.runtime=MD release
        conan profile update settings.arch=x86_64 release
        conan profile update settings.os=Windows release
    fi
fi

# Install dependencies using Conan
conan install . --build=missing || { echo "Conan install failed"; exit 1; }

# Build the project with the 'release' profile
if [[ "$OSTYPE" == "msys" ]]; then
    meson setup builddir --backend=vs2017 || { echo "Meson setup failed"; exit 1; }
else
    meson setup builddir || { echo "Meson setup failed"; exit 1; }
fi

meson compile -C builddir || { echo "Build failed"; exit 1; }

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
