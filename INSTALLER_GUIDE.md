# PSIO Assist EX - Windows Installer Guide

## Overview

This guide explains how to generate a Windows `.exe` installer for your PSIO Assist EX project from Linux Mint using GitHub Actions.

## What's Been Set Up

### 1. GitHub Actions Workflow
- **File**: `.github/workflows/build-windows.yml`
- **Purpose**: Automatically builds your Qt application for Windows and creates an installer
- **Triggers**: 
  - Manual execution (workflow_dispatch)
  - When you push a tag starting with 'v' (e.g., v1.0.0)

### 2. NSIS Installer Script
- **File**: `installer/psio-assist-installer.nsi`
- **Purpose**: Creates a professional Windows installer with:
  - Desktop and Start Menu shortcuts
  - Proper uninstaller
  - Registry entries for Add/Remove Programs
  - Qt dependencies included

### 3. Test Script
- **File**: `test-workflow.sh`
- **Purpose**: Verifies that all required files are present and syntax is correct

## How to Use

### Step 1: Commit and Push Your Changes
```bash
git add .
git commit -m "Add Windows installer support"
git push origin main
```

### Step 2: Run the Workflow
1. Go to your GitHub repository
2. Click on the "Actions" tab
3. Find "Build Windows Installer" workflow
4. Click "Run workflow" button
5. Select the branch (usually "main")
6. Click "Run workflow"

### Step 3: Download the Installer
1. Wait for the workflow to complete (usually 5-10 minutes)
2. Go to the completed workflow run
3. Scroll down to "Artifacts" section
4. Download "windows-installer" artifact
5. Extract the ZIP file to get `PSIO-Assist-EX-Installer.exe`

## What the Workflow Does

1. **Sets up Qt 6.5.0** with MinGW compiler
2. **Builds your application** using CMake
3. **Packages the application** with all Qt dependencies
4. **Installs NSIS** (Nullsoft Scriptable Install System)
5. **Creates the installer** using the NSIS script
6. **Uploads the installer** as a downloadable artifact

## Features of the Generated Installer

- **Professional appearance** with modern Windows installer UI
- **Desktop shortcut** for easy access
- **Start Menu entry** with uninstall option
- **Proper uninstaller** that removes all files and registry entries
- **Qt dependencies included** - no need to install Qt separately
- **Translation files included** - supports both English and Portuguese
- **Administrator privileges** - ensures proper installation

## Troubleshooting

### If the workflow fails:
1. Check the workflow logs for specific error messages
2. Common issues:
   - Missing translation files (`.qm` files)
   - CMake configuration problems
   - NSIS script syntax errors

### If the installer doesn't work:
1. Make sure you're running it as Administrator
2. Check Windows Defender or antivirus software
3. Verify that all Qt DLLs are included

## Local Testing

You can test the NSIS script locally:
```bash
# Test the script syntax
makensis -HDRINFO installer/psio-assist-installer-test.nsi

# Run the full test script
./test-workflow.sh
```

## Customization

### Changing the Installer Appearance
Edit `installer/psio-assist-installer.nsi`:
- Modify `!define APPNAME` for the application name
- Change `!define COMPANYNAME` for your company
- Update version numbers in the `!define VERSION*` lines

### Adding More Files
In the NSIS script, add more `file` commands:
```nsis
file "path\to\your\file"
```

### Changing Installation Directory
Modify the `InstallDir` line:
```nsis
InstallDir "$PROGRAMFILES\YourAppName"
```

## Alternative Methods

If GitHub Actions doesn't work for you, you can also:

1. **Use a Windows VM** with Qt and NSIS installed
2. **Use Docker** with a Windows container
3. **Use a CI/CD service** like Azure DevOps or GitLab CI
4. **Cross-compile** using MXE (M cross environment)

## Support

If you encounter issues:
1. Check the GitHub Actions logs
2. Verify all files are committed and pushed
3. Make sure your repository is public (for free GitHub Actions)
4. Check that you have the necessary permissions

## Next Steps

1. **Test the installer** on a Windows machine
2. **Create a release** on GitHub with the installer
3. **Add an icon** to the installer (optional)
4. **Customize the installer** appearance (optional)
5. **Set up automatic releases** when you push tags

The installer is now ready to use! 🎉
