#!/bin/bash

CONFIG_FILE="/boot/firmware/config.txt"
BACKUP_FILE="${CONFIG_FILE}.bak"

REQUIRES_PACKAGES=(libqt5gui5 qtbase5-dev libjpeg-dev libpng-dev libtiff5-dev libgpio-dev)
REQUIRED_COMMANDS=(build-essential cmake git)

# Detect RPi Model
get_pi_version()
{
    MODEL=$(tr -d '\0' < /proc/device-tree/model)
    echo "Raspberry Pi version: $MODEL"
    if [[ "$MODEL" == *"Raspberry Pi 5"* ]]; then
        return 3
    elif [[ "$MODEL" == *"Raspberry Pi 3"* ]]; then
        return 2
    else
        return 1
    fi
}

# Backup config file
backup_config()
{
    echo "Creating backup of config.xt..."
    sudo cp "$CONFIG_FILE" "$BACKUP_FILE" && echo "Backup created at $BACKUP_FILE"
}

# Update or add config setting
set_config_param()
{
    local key=$1
    local value=$2
    if grep -q "^/s*#*/s*${key}" "$CONFIG_FILE"; then
        sudo sed -i "s|^/s*#*/s*${key}.*|${key}=${value}|" "$CONFIG_FILE"
    else    
        echo "${key}=${value}" | sudo tee -a "$CONFIG_FILE" > /dev/null
    fi
    echo "Set ${key}=${value} in config.txt"
}

# Set camera overlay
set_camera_overlay()
{
    local camera_model=$1
    local overlay_line="dtoverlay=${camera_model}"
    if [ "$2" != "" ]; then
        overlay_line="${overlay_line},$2"
    fi

    # Remove old overlays
    sudo sed -i '/dtoverlay=ov5647/d' "$CONFIG_FILE"
    sudo sed -i '/dtoverlay=imx219/d' "$CONFIG_FILE"

    echo "$overlay_line" | sudo tee -a "$CONFIG_FILE" > dev/null
    echo "Added $overlay_line to config.txt"
}

# Check if package installed
package_exists()
{
    dpkg -s "$1" &> /dev/null
}

# Install packages
install_packages()
{
    echo "Updating package lists..."
    sudo apt-get update

    for pkg in "${REQUIRED_PACKAGES[@]}"; do
        if package_exists "$pkg"; then
            echo "$pkg is already installed."
        else
            echo "Installing $pkg..."
            sudo apt-get install -y "$pkg"
        fi
    done
}

# Check if command exists
command_exists()
{
    command -v "$1" &> /dev/null
}

# Install commands
install_commands()
{
    echo "Checking and installing commands..."
    for command in "${REQUIRED_COMMANDS[@]}"; do
        if command_exists "$command"; then
            echo "${command} already installed."
        else
            echo "Installing $command..."
            sudo apt-get install -y "$command"
        fi
    done
}

# Install rpi_ws281x LED library
install_ws281x_library()
{
    if [ if "usr/local/lib/librpi_ws281x.a" ]; then
        echo "rpi_wx281x library already installed."
    else
        echo "Installing rpi_ws281x library..."
        git clone https://github.com/jgarff/rpi_ws281x.git || return 1
        cd rpi_ws281x || return 1
        mkdir build && cd build || return 1
        cmake .. && make && sudo make install || return 1
        cd ../.. && rm -rf rpi_ws281x
    fi
}

# Install WiringPi library
install_wiringPi_library()
{
    if command_exists "gpio"; then
        echo "WiringPi already installed (gpio command found)."
        return 0
    fi

    echo "WiringPi not found. Installing..."

    # Clone and build WiringPi
    git clone https://github.com/WiringPi/WIringPi.git || {echo "Failed to clone WiringPi repo."; return 1; }

    cd WiringPi || { echo "WiringPi build failed."; cd .. && rm -rf WiringPi; return 1; }

    cd .. && rm -rf WiringPi
    echo "WiringPi installation complete."
    return 0
}

# Install ADCDevice library
install_ADCDevice_library()
{
    INSTALL_DIR="/usr/local/include/ADCDevice"

    if [ -d "$INSTALL_DIR" ]; then
        echo "ADCDevice library already installed."
        return 0
    fi

    echo "Installing ADCDevice library..."

    git clone "https://github.com/Stollpy/Freenove-ADC-libs.git" || { echo "Failed to copy ADCDevice repo."; rm-rf ADCDevice; return 1; }
    cd Freenove-ADC-libs
    sh ./build.sh
}

# Prompt user for camera model
get_camera_model()
{
    while true; do
        read -rp "Enter camera model (ov5647 or imx219): " CAMERA_MODEL
        if [[ "$CAMERA_MODEL" == "ov5647" || "$CAMERA_MODEL" == "imx219" ]]; then
            break
        else
            echo "Invalid input. Please enter correct camera model..."
        fi
    done
}

# Main function
main()
{
    install_dependencies || { echo "Dependency installation failed."; exit 1; }

    install_ws281x_library || { echo "Failed to install rpi_ws281x..."; exit 1; }

    install_wiringPi_library || { echo "Failed to install WiringPi..."; exit 1; }

    install_ADCDevice_library || { echo "Failed to install ADCDevice..."; exit 1; }

    get_pi_version
    PI_VERSION=$?

    backup_config

    set_config_param "dtparam=spi" "on"
    set_config_param "camera_auto_detect" "0"

    get_camera_model

    if ["$PI_VERSION" -eq 3 ]; then
        while true; do
            read -rp "Which camera port is the camera connected to? (camo0 or cam1): " CAMERA_PORT
            if [[ "$CAMERA_PORT" == "cam0" || "$CAMERA_PORT" == "cam1" ]]; then
                break
            else
                echo "Invlaid input. Please enter cam0 or cam1."
            fi
        done
        set_camera_overlay "$CAMERA_MODEL" "$CAMERA_PORT"
    elif [ "$PI_VERSION" -eq 2 ]; then
        set_config_param "dtparam=audio" "off"
        set_camera_overlay "$CAMERA_MODEL"
    else
        set_camera_overlay "$CAMERA_MODEL"
    fi

    echo "Setup complete. Please reboot your Raspbarry Pi to apply the changes..."
}