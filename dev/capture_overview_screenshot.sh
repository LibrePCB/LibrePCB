#!/bin/bash

# Helper script to capture screenshots of LibrePCB windows.
# Requirements: sudo apt-get install wmctrl shutter imagemagick
# Note: Enable round corners in shutter preferences!

set -euo pipefail

WIDTH=800
HEIGHT=500
X1=100
X2=280
X3=460
Y1=100
Y2=250
Y3=400

# capture screenshots
wmctrl -a "Control Panel" && wmctrl -r "Control Panel" -e 0,$X1,$Y1,$WIDTH,$HEIGHT
shutter -w="Control Panel" -o control_panel_small.png -d 3 -c -e
wmctrl -a "Board Editor" && wmctrl -r "Board Editor" -e 0,$X2,$Y2,$WIDTH,$HEIGHT
shutter -w="Board Editor" -o board_editor_small.png -d 3 -c -e
wmctrl -a "Schematic Editor" && wmctrl -r "Schematic Editor" -e 0,$X3,$Y3,$WIDTH,$HEIGHT
shutter -w="Schematic Editor" -o schematic_editor_small.png -d 3 -c -e

# merge screenshots together
convert -page +$X1+$Y1 control_panel_small.png \
        -page +$X2+$Y2 board_editor_small.png \
        -page +$X3+$Y3 schematic_editor_small.png \
        -background transparent -layers merge +repage overview_small.png

