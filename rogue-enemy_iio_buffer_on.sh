#!/bin/bash
modprobe industrialio-sw-trigger
modprobe iio-trig-sysfs
modprobe iio-trig-hrtimer

# hrtimer
if [ ! -d "/home/config" ]; then
    mkdir -p /home/config
fi

mount -t configfs none /home/config
mkdir -p /home/config/iio/triggers/hrtimer/rogue

# set sampling frequency for rogue
for i in /sys/bus/iio/devices/* ; do
  if [ -d "$i" ]; then
    if [ -f "$i/name" ]; then
      name=$(cat "$i/name")
      if [ "$name" = "rogue" ]; then
        echo "1600" > "$i/sampling_frequency"
      fi
    fi
  fi
done

# set the gyroscope
for i in /sys/bus/iio/devices/* ; do
  if [ -d "$i" ]; then
    if [ -f "$i/name" ]; then
      name=$(cat "$i/name")
      if [ "$name" = "bmi323-imu" ]; then

        # change chip sampling frequency
        echo "1600.000000" > "$i/in_accel_sampling_frequency"
        echo "1600.000000" > "$i/in_anglvel_sampling_frequency"

        # enable accel data acquisition
        echo 1 > "$i/scan_elements/in_accel_x_en"
        echo 1 > "$i/scan_elements/in_accel_y_en"
        echo 1 > "$i/scan_elements/in_accel_z_en"

        # enable gyroscope data acquisition
        echo 1 > "$i/scan_elements/in_anglvel_x_en"
        echo 1 > "$i/scan_elements/in_anglvel_y_en"
        echo 1 > "$i/scan_elements/in_anglvel_z_en"

        # enable timestamp reporting
        echo 1 > "$i/scan_elements/in_timestamp_en"

        # bind rogue hrtimer to to the iio device
        echo "rogue" > "$i/trigger/current_trigger"

        # enable the buffer
        echo 1 > "$i/buffer0/enable"

        echo "bmi323-imu buffer started"
      fi
    fi
  fi
done
