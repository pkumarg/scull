#!/bin/sh
module="scull"
device="scull"
mode="664"

/sbin/rmmod ./$module.ko
rm -f /dev/${device}0
