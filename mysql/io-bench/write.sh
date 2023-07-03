#!/bin/bash -e

mkdir -p /tmp/__ramdisk__
sudo mount -t tmpfs -o size=3G tmpfs /tmp/__ramdisk__

make write
./write Z
rm -f write

sudo umount /tmp/__ramdisk__
