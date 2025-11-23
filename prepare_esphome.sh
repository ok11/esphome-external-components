#!/bin/bash

ESPHOME_PATH="${1}"
ESPHOME_TAG="${2:-latest}"
SCRIPT_PATH=$(pwd)

if [ -z "$ESPHOME_PATH" ]; then
  echo "Usage: $0 <path_to_esphome>"
  exit 1
fi

echo "Preparing ESPHome at $ESPHOME_PATH"

ESPHOME_ROOT="$(dirname "$ESPHOME_PATH")"

if [ ! -d "$ESPHOME_PATH" ]; then
    echo "Cloning ESPHome repository into $ESPHOME_PATH:Q..."
    git clone https://github.com/esphome/esphome.git --branch "$ESPHOME_TAG" "$ESPHOME_PATH"
else
    echo "ESPHome repository already exists, pulling latest changes..."
    cd "$ESPHOME_PATH"
    git pull origin dev
    cd -
    echo "Done."
fi

cd "$ESPHOME_PATH"

if [ ! -d "$ESPHOME_PATH/venv" ]; then
    echo "Setting up ESPHome at $ESPHOME_PATH..."
    ./script/setup
    echo "Done."
else
    echo "ESPHome is already set up."
fi

source venv/bin/activate

if [ ! -d "$ESPHOME_PATH/.pio/libdeps" ]; then
    echo "Installing PlatformIO dependencies at $ESPHOME_PATH..."
    "$SCRIPT_PATH/platformio_install_deps_locally.py" "$ESPHOME_PATH/platformio.ini"
    echo "Done."
else
    echo "PlatformIO dependencies are already installed."
fi

cd -

echo "ESPHome preparation complete."
