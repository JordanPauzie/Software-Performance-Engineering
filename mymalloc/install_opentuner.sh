#!/bin/bash
set -e

if [[ $EUID -ne 0 ]]; then
   sudo -E $0
else
  echo "Installing OpenTuner"
  apt install python3-pip
  apt install python3-venv
  python3 -m venv homework7-venv
  source homework7-venv/bin/activate
  pip3 install SQLAlchemy==1.4.54
  pip3 install opentuner
  echo "OpenTuner installed!"
fi