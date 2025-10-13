#!/bin/bash

# PSIO Assist EX - Windows Installer Build Script
# This script builds a Windows installer from Linux using cross-compilation

set -e

echo "=== PSIO Assist EX - Windows Installer Builder ==="

# Configuration
PROJECT_NAME="psio-assist-ex"
VERSION="1.0.0"
BUILD_DIR="build-windows"
INSTALLER_DIR="installer"
DIST_DIR="dist"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    print_error "CMakeLists.txt not found. Please run this script from the project root."
    exit 1
fi

print_status "Starting Windows installer build process..."

# Clean previous builds
print_status "Cleaning previous builds..."
rm -rf "$BUILD_DIR" "$DIST_DIR"
mkdir -p "$BUILD_DIR" "$DIST_DIR"

# Method 1: Using MXE (M cross Environment) - Recommended
build_with_mxe() {
    print_status "Building with MXE (M cross Environment)..."
    
    # Check if MXE is available
    if command -v x86_64-w64-mingw32.static-cmake &> /dev/null; then
        print_status "MXE found, using cross-compilation..."
        
        cd "$BUILD_DIR"
        
        # Configure with MXE
        x86_64-w64-mingw32.static-cmake \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX=../"$DIST_DIR" \
            ..
        
        # Build
        make -j$(nproc)
        
        # Install
        make install
        
        cd ..
        
        print_status "Cross-compilation completed successfully!"
        return 0
    else
        print_warning "MXE not found. Please install MXE for cross-compilation."
        return 1
    fi
}

# Method 2: Using Docker with Windows build environment
build_with_docker() {
    print_status "Building with Docker..."
    
    # Check if Docker is available
    if ! command -v docker &> /dev/null; then
        print_warning "Docker not found. Please install Docker."
        return 1
    fi
    
    # Create Dockerfile for Windows build
    cat > Dockerfile.windows << 'EOF'
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    qt6-base-dev \
    qt6-tools-dev \
    mingw-w64 \
    wine \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Build for Windows
RUN mkdir build && cd build && \
    x86_64-w64-mingw32-cmake .. && \
    make -j$(nproc)

# Copy built files
RUN cp build/psio-assist.exe /output/
EOF

    # Build Docker image and run
    docker build -f Dockerfile.windows -t psio-assist-builder .
    docker run --rm -v "$(pwd)/$DIST_DIR:/output" psio-assist-builder
    
    print_status "Docker build completed!"
    return 0
}

# Method 3: Using Wine (if available)
build_with_wine() {
    print_status "Building with Wine..."
    
    if ! command -v wine &> /dev/null; then
        print_warning "Wine not found. Please install Wine."
        return 1
    fi
    
    # This would require setting up a Windows Qt environment in Wine
    print_warning "Wine build method not fully implemented yet."
    return 1
}

# Try different build methods
if build_with_mxe; then
    BUILD_METHOD="mxe"
elif build_with_docker; then
    BUILD_METHOD="docker"
elif build_with_wine; then
    BUILD_METHOD="wine"
else
    print_error "No suitable build method found!"
    print_error "Please install one of the following:"
    print_error "  - MXE (M cross Environment)"
    print_error "  - Docker"
    print_error "  - Wine with Windows Qt"
    exit 1
fi

print_status "Build method used: $BUILD_METHOD"

# Copy additional files to dist
print_status "Copying additional files..."
cp -r translations "$DIST_DIR/"
cp README.md "$DIST_DIR/"
cp LICENSE "$DIST_DIR/"

# Create installer
print_status "Creating Windows installer..."

# Check if NSIS is available
if command -v makensis &> /dev/null; then
    print_status "Creating NSIS installer..."
    
    # Copy installer script to dist
    cp "$INSTALLER_DIR/psio-assist-installer.nsi" "$DIST_DIR/"
    
    # Create installer
    cd "$DIST_DIR"
    makensis psio-assist-installer.nsi
    
    # Move installer to project root
    mv "PSIO-Assist-EX-Installer.exe" "../"
    cd ..
    
    print_status "NSIS installer created: PSIO-Assist-EX-Installer.exe"
else
    print_warning "NSIS not found. Installer script created but not compiled."
    print_warning "To create the installer, install NSIS and run:"
    print_warning "  makensis $DIST_DIR/psio-assist-installer.nsi"
fi

# Create portable version
print_status "Creating portable version..."
cp -r "$DIST_DIR" "PSIO-Assist-EX-Portable"
cd "PSIO-Assist-EX-Portable"
zip -r "../PSIO-Assist-EX-Portable.zip" .
cd ..
rm -rf "PSIO-Assist-EX-Portable"

print_status "Portable version created: PSIO-Assist-EX-Portable.zip"

# Summary
print_status "Build completed successfully!"
echo ""
echo "Generated files:"
echo "  - $DIST_DIR/ (Windows application files)"
if [ -f "PSIO-Assist-EX-Installer.exe" ]; then
    echo "  - PSIO-Assist-EX-Installer.exe (Windows installer)"
fi
echo "  - PSIO-Assist-EX-Portable.zip (Portable version)"
echo ""
echo "To test the installer on Windows:"
echo "  1. Copy PSIO-Assist-EX-Installer.exe to a Windows machine"
echo "  2. Run the installer as administrator"
echo "  3. The application will be installed with desktop and start menu shortcuts"
