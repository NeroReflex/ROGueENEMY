#!/bin/bash

for i in /sys/bus/iio/devices/* ; do
  if [ -d "$i" ]; then
    if [ -f "$i/name" ]; then
      name=$(cat "$i/name")
      if [ "$name" = "bmi323-imu" ]; then
        # bind fake hrtimer to to the iio device
        echo "void" > "$i/trigger/current_trigger"

        # enable the buffer
        echo 0 > "$i/buffer0/enable"

        echo "bmi323-imu buffer started"
      fi
    fi
  fi
done
