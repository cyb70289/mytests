#!/bin/bash

myip=$(hostname -I | cut -d' ' -f1)
echo "on test node:"
echo "sudo offcputime-bpfcc -df -p PID 5 > /tmp/__offcpu.stacks"
echo "cat /tmp/__offcpu.stacks | nc -w1 ${myip} 6789'"

nc -l -p 6789 > /tmp/offcpu-stack
stat --printf="offcpu file size = %s\n" /tmp/offcpu-stack

echo "generating svg..."
~/work/FlameGraph/flamegraph.pl --color=io --countname=us < /tmp/offcpu-stack > /tmp/offcpu.svg
if [ $? != 0 ]; then
    echo "failed!"
    exit 1
fi

echo "saved to /tmp/offcpu.svg"
firefox /tmp/offcpu.svg > /dev/null 2>&1 &
