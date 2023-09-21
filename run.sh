
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=install
cmake --build build -- install -j4
source install/setup.sh
dd_web_display --export $DETECTOR_PATH/epic_calorimeters.xml
checkOverlaps -t 0.0001 -c epic_calorimeters.xml > overlaps.txt
