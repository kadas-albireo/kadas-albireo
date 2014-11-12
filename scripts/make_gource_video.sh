#!/bin/bash
###########################################################################
#    make_gource_video.sh
#    ---------------------
#    Date                 : October 2011
#    Copyright            : (C) 2011 by Tim Sutton
#    Email                : tim at kartoza dot com
###########################################################################
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
###########################################################################


echo "A script to generate a source video progression"
echo "see http://woostuff.wordpress.com/2011/01/03/generating-a-gource-source-commit-history-visualization-for-qgis-quantum-gis/"
echo "Run it from the root directory e.g. scripts/$0"

gource --title "QGIS" --logo images/icons/qgis-icon.png \
    --hide filenames \
    --date-format "%d, %B %Y" \
    --seconds-per-day 0.05 \
    --highlight-all-users \
    --auto-skip-seconds 0.5 \
    --file-idle-time 0 \
    --max-files 999999999 \
    --multi-sampling \
    --stop-at-end \
    --elasticity 0.1 \
    -b ffffff \
    --disable-progress \
    --user-friction .2 \
    --output-ppm-stream - | \
 ffmpeg -an -threads 4 -y -b 3000K -vb 8000000 -r 60 -f image2pipe \
        -vcodec ppm -i - -vcodec libx264 -vpre libx264-medium qgis-history.mp4
