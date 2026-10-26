#!/bin/bash
set -e

echo "Installing dependencies"
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
    openjdk-8-jre \
    openjdk-8-jdk \
    python3 \
    python3-dev \
    python3-pip \
    python3-setuptools \
    python3-wheel \
    tcl

echo "Building the tester"
cd ../tester
make

echo "Install gui"
curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.7/install.sh | bash
source ~/.nvm/nvm.sh && nvm install 20.12.2
cd ../lcgui
npm install
cd ./packages/shared
npm run build

echo "Finished setup"