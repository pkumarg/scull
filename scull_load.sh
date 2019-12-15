#!/bin/sh
module="scull"
device="scull"
mode="664"

# invoke insmod with all arguments we got
# and use a pathname, as newer modutils don't look in . by default
/sbin/insmod ./$module.ko $* || exit 1

# remove stale nodes
rm -f /dev/${device}0

major=`grep $device /proc/devices | cut -d " " -f1`
echo "Major is $major"

mknod /dev/${device}0 c $major 0

# give appropriate group/permissions, and change the group.
# Not all distributions have staff, some have "wheel" instead.
group="pkumar"
grep -q '^pkumar:' /etc/group || echo "Group pkumar doesn't exists"

chgrp $group /dev/${device}0
chmod $mode /dev/${device}0
