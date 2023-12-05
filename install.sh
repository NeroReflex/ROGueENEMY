#!/bin/bash

# Define variables
REPO_URL="https://github.com/corando98/ROGueENEMY"
BUILD_DIR="/tmp/ROGueENEMY"
INSTALL_DIR="/usr/bin"
SYSTEMD_DIR="/etc/systemd/system"
UDEV_RULES_DIR="/usr/lib/udev/rules.d"
CONFIG_DIR="/etc/ROGueENEMY"

# Ensure running as root
if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root." >&2
    exit 1
fi

# Install dependencies
# apt-get update
# apt-get install -y cmake libconfig-dev libevdev libconfig git

# Preparation
rm -rf "$BUILD_DIR"
git clone "$REPO_URL" "$BUILD_DIR"

cp "$BUILD_DIR/rogue_enemy.rule" "$UDEV_RULES_DIR/99-rogue_enemy.rules"
cp "$BUILD_DIR/80-playstation.rules" "$UDEV_RULES_DIR"

curl -L $(curl -s https://api.github.com/repos/corando98/ROGueENEMY/releases/latest | grep "browser_download_url" | cut -d '"' -f 4) -o $BUILD_DIR/rogue-enemy

# Installation
mkdir -p "$INSTALL_DIR"
cp "$BUILD_DIR/rogue-enemy" "$INSTALL_DIR/rogue-enemy"

mkdir -p "$SYSTEMD_DIR"
mkdir -p "$UDEV_RULES_DIR"
mkdir -p "$CONFIG_DIR"

install -m 644 "$BUILD_DIR/rogue-enemy.service" "$SYSTEMD_DIR/"
install -m 644 "$BUILD_DIR/80-playstation.rules" "$UDEV_RULES_DIR/"
install -m 644 "$BUILD_DIR/99-rogue_enemy.rules" "$UDEV_RULES_DIR/"
# install -m 644 "$BUILD_DIR/config.cfg" "$CONFIG_DIR/config.cfg"

# Post-installation
systemctl daemon-reload
systemctl enable rogue-enemy.service
systemctl start rogue-enemy.service
udevadm control --reload-rules
udevadm trigger

# Cleanup
rm -rf "$BUILD_DIR"

echo "Installation complete."

