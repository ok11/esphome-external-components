#!/bin/bash

source ./.env

ESPHOME_DIR="${1:-$ESPHOME_PATH}"
ESPHOME_TAG="${2:-latest}"
SCRIPT_DIR=$(pwd)
PIO_LIBDEPS_PATH="$ESPHOME_DIR/.pio/libdeps"

if [ -z "$ESPHOME_DIR" ]; then
  echo "Usage: $0 <path_to_esphome> [esphome_tag] (or setting ESPHOME_PATH environment variable)"
  exit 1
fi

echo "Preparing ESPHome at $ESPHOME_DIR"

cd "$ESPHOME_DIR"

if [ ! -d "$ESPHOME_DIR" ]; then
    echo "Cloning ESPHome repository into $ESPHOME_DIR..."
    git clone https://github.com/esphome/esphome.git --branch "$ESPHOME_TAG" "$ESPHOME_DIR"
else
    echo "ESPHome repository already exists, pulling latest changes..."
    old_head=$(git rev-parse HEAD)
    git pull origin dev
    new_head=$(git rev-parse HEAD)

    if [ "$old_head" != "$new_head" ]; then
        echo "Changes were pulled, deleting PlatformIO dependencies to ensure a clean state..."
        if [ -d "$PIO_LIBDEPS_PATH" ]; then
            rm -rf "$PIO_LIBDEPS_PATH"
        fi
    fi
    echo "Done."
fi

if [ ! -d "$ESPHOME_DIR/venv" ]; then
    echo "Setting up ESPHome at $ESPHOME_DIR..."
    ./script/setup
    echo "Done."
else
    echo "ESPHome is already set up."
fi

source venv/bin/activate

if [ ! -d "$PIO_LIBDEPS_PATH" ]; then
    echo "Installing PlatformIO dependencies at $ESPHOME_DIR..."
    "$SCRIPT_DIR/platformio_install_deps_locally.py" "$ESPHOME_DIR/platformio.ini"
    echo "Done."
else
    echo "PlatformIO dependencies are already installed."
fi

cd -

echo "ESPHome preparation complete."
