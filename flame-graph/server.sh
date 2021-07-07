#!/bin/bash

# client
# $ sudo perf record -g ...
# $ sudo sh -c 'perf script | nc -w1 a015565 6789'

echo "run below command on client after perf record"
echo "sudo sh -c 'perf script | nc -w1 $(hostname) 6789'"

nc -l -p 6789 > /tmp/perf-script
stat --printf="perf script size = %s\n" /tmp/perf-script

echo "generating svg..."
~/work/FlameGraph/stackcollapse-perf.pl /tmp/perf-script | ~/work/FlameGraph/flamegraph.pl > /tmp/perf.svg
if [ $? != 0 ]; then
    echo "failed!"
    exit 1
fi

echo "saved to /tmp/perf.svg"
firefox /tmp/perf.svg > /dev/null 2>&1 &
