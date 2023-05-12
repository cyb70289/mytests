#!/bin/bash

myip=$(hostname -I | cut -d' ' -f1)
echo "on test node:"
echo "sudo perf record -g ..."
echo "sudo sh -c 'perf script | nc -w1 ${myip} 6789'"

nc -l -p 6789 > /tmp/perf-script
stat --printf="perf script size = %s\n" /tmp/perf-script

echo "generating svg..."
~/work/FlameGraph/stackcollapse-perf.pl /tmp/perf-script | ~/work/FlameGraph/flamegraph.pl --title "On CPU" > /tmp/oncpu.svg
if [ $? != 0 ]; then
    echo "failed!"
    exit 1
fi

echo "saved to /tmp/oncpu.svg"
firefox /tmp/oncpu.svg > /dev/null 2>&1 &
