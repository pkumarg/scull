#!/bin/sh
module="scull"
device="scull"
MODE="664"

# invoke insmod with all arguments we got
# and use a pathname, as newer modutils don't look in . by default
/sbin/insmod ./$module.ko $* || exit 1

# remove stale nodes
rm -f /dev/${device}0

major=`grep $device /proc/devices | cut -d " " -f1`
echo "Major is $major"

mknod /dev/${device}0 c $major 0

grep -q "^${USER}:" /etc/group || echo "Group ${USER} doesn't exists"

chgrp $USER /dev/${device}0
chmod $MODE /dev/${device}0
