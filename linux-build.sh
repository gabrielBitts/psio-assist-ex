#!/bin/bash

# Check if build directory exists, if not, create it
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir -p build
fi

# Enter build directory
echo "Entering build directory..."
cd build

# Configure CMake
echo "Configuring CMake..."
cmake ..

# Compile the project
echo "Compiling..."
cmake --build . -j$(nproc)

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Build completed successfully!"
    echo "Executable is in build/psio-assist"
else
    echo "Error during build!"
    cd ..
    exit 1
fi

# Return to previous directory
echo "Returning to main directory..."
cd .. 
