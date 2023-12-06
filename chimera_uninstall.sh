#!/bin/bash

# Define the user's home directory
# This is needed to find the installed executable in the user's home directory
if [ "$SUDO_USER" ]; then
    HOME_DIR=$(getent passwd "$SUDO_USER" | cut -d: -f6)
else
    HOME_DIR=$HOME
fi

# Define variables
INSTALL_DIR="$HOME_DIR/.local/bin"
SYSTEMD_DIR="/etc/systemd/system"
UDEV_RULES_DIR="/etc/udev/rules.d"
CONFIG_DIR="/etc/ROGueENEMY"

echo "Starting uninstallation of ROGueENEMY..."

# Ensure running as root
if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root." >&2
    exit 1
fi

# Stop and disable the systemd service
echo "Disabling and stopping the systemd service..."
systemctl stop rogue-enemy.service
systemctl disable rogue-enemy.service
rm -f "$SYSTEMD_DIR/rogue-enemy.service"

# Remove the executable
echo "Removing the executable..."
rm -f "$INSTALL_DIR/rogue-enemy"

# Remove udev rules
echo "Removing udev rules..."
rm -f "$UDEV_RULES_DIR/99-rogue_enemy.rules"
rm -f "$UDEV_RULES_DIR/80-playstation.rules"

# Remove configuration directory
echo "Removing configuration files..."
rm -rf "$CONFIG_DIR"

# Reload system daemons and udev rules
echo "Reloading system daemons and udev rules..."
systemctl daemon-reload
udevadm control --reload-rules
udevadm trigger

# Cleanup
echo "Uninstallation complete."

