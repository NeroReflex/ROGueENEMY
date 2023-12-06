#!/bin/bash

# Define variables
REPO_URL="https://github.com/corando98/ROGueENEMY"
BUILD_DIR="/tmp/ROGueENEMY"
INSTALL_DIR="/usr/bin"
SYSTEMD_DIR="/etc/systemd/system"
UDEV_RULES_DIR="/usr/lib/udev/rules.d"
CONFIG_DIR="/etc/ROGueENEMY"

echo "Starting installation of ROGueENEMY..."

# Ensure running as root
if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root." >&2
    exit 1
fi

echo "Installing dependencies..."
# apt-get update
# apt-get install -y cmake libconfig-dev libevdev libconfig git

# Preparation
echo "Preparing installation..."
echo "Preparing installation..."
rm -rf "$BUILD_DIR"
git clone "$REPO_URL" "$BUILD_DIR"

echo "Copying udev rules..."
cp "$BUILD_DIR/rogue_enemy.rule" "$UDEV_RULES_DIR/99-rogue_enemy.rules"
cp "$BUILD_DIR/80-playstation.rules" "$UDEV_RULES_DIR"

echo "Downloading the latest release..."
echo "Downloading the latest release..."
curl -L $(curl -s https://api.github.com/repos/corando98/ROGueENEMY/releases/latest | grep "browser_download_url" | cut -d '"' -f 4) -o $BUILD_DIR/rogue-enemy

# Installation
echo "Installing executable..."
mkdir -p "$INSTALL_DIR"
cp "$BUILD_DIR/rogue-enemy" "$INSTALL_DIR/rogue-enemy"

echo "Creating and setting up system service..."
echo "Setting up system service..."
mkdir -p "$SYSTEMD_DIR"
mkdir -p "$UDEV_RULES_DIR"
mkdir -p "$CONFIG_DIR"

# Create systemd service file
cat <<EOF > "$SYSTEMD_DIR/rogue-enemy.service"
[Unit]
Description=ROGueENEMY service

[Service]
Type=simple
Nice=-15
Restart=always
RestartSec=5
WorkingDirectory=$INSTALL_DIR
ExecStart=$INSTALL_DIR/rogue-enemy

[Install]
WantedBy=multi-user.target
EOF

# Install other files
install -m 644 "$BUILD_DIR/80-playstation.rules" "$UDEV_RULES_DIR/"
install -m 644 "$BUILD_DIR/99-rogue_enemy.rules" "$UDEV_RULES_DIR/"
# Uncomment the following if the configuration file is necessary
# install -m 644 "$BUILD_DIR/config.cfg" "$CONFIG_DIR/config.cfg"

# Post-installation
echo "Reloading system daemons and applying new udev rules..."
systemctl daemon-reload
systemctl enable rogue-enemy.service
systemctl start rogue-enemy.service
udevadm control --reload-rules
udevadm trigger

# Cleanup
echo "Cleaning up..."
rm -rf "$BUILD_DIR"

echo "Installation complete."
