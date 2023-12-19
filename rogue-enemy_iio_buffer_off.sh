#!/bin/bash
cd /sys/bus/iio/devices/iio\:device0
echo "void" > trigger/current_trigger
echo 0 > buffer0/enable