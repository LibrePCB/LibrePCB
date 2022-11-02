#!/bin/bash

# Helper script to capture screenshots of LibrePCB windows.
# Requirements: sudo apt-get install wmctrl shutter imagemagick
# Note: Enable round corners in shutter preferences!

set -euo pipefail

function capture_screenshot {
  echo "----- NEXT WINDOW: $1 -----"
  sleep 5
  wmctrl -a "$1" && wmctrl -r "$1" -e 0,100,100,1100,650
  shutter -w="$1" -o "`pwd`/$2" -d 3 -c -e -n
  sleep 5
  convert -thumbnail 770x455 "$2" "thumbs/$2"
}

mkdir -p thumbs
capture_screenshot "Control Panel" "control_panel.png"
capture_screenshot "Board Editor" "board_editor.png"
capture_screenshot "Schematic Editor" "schematic_editor.png"
capture_screenshot "Add Component" "add_component_dialog.png"
capture_screenshot "Library Manager" "library_manager.png"
capture_screenshot "Library Editor" "library_editor.png"
