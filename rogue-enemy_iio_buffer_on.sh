#!/bin/bash
modprobe industrialio-sw-trigger
modprobe iio-trig-sysfs
modprobe iio-trig-hrtimer

# hrtimer
if [! -d "/home/config"]; then
    mkdir /home/config
fi

mount -t configfs none /home/config
mkdir /home/config/iio/triggers/hrtimer/rogue

cd /sys/bus/iio/devices/iio\:device0
echo 1 > scan_elements/in_accel_x_en
echo 1 > scan_elements/in_accel_y_en
echo 1 > scan_elements/in_accel_z_en
echo 1 > scan_elements/in_anglvel_x_en
echo 1 > scan_elements/in_anglvel_y_en
echo 1 > scan_elements/in_anglvel_z_en
echo 1 > scan_elements/in_timestamp_en
echo "rogue" > trigger/current_trigger
echo 1 > buffer0/enable