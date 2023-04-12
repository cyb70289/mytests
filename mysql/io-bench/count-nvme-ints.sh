#!/bin/bash

: ${DEV:=nvme5q}
: ${CNT:=128}

echo count ${DEV} interrupts for each queue
for i in $(seq 0 ${CNT}); do
    mydev="${DEV}${i}"
    sum=0
    for c in $(grep "\<${mydev}\>" /proc/interrupts); do
        if [[ ${c} =~ ^[0-9]+$ ]]; then
            sum=$((sum + c))
        fi
    done
    eval ${mydev}=${sum}
done

echo run the test
./mydd "$@"

echo count and diff ${DEV} interrupts for each queue
diff=0
for i in $(seq 0 ${CNT}); do
    mydev="${DEV}${i}"
    sum=0
    for c in $(grep "\<${mydev}\>" /proc/interrupts); do
        if [[ ${c} =~ ^[0-9]+$ ]]; then
            sum=$((sum + c))
        fi
    done
    eval old_sum='$'${mydev}
    if [ ${sum} -gt ${old_sum} ]; then
        diff1=$((sum - old_sum))
        diff=$((diff + diff1))
        echo "${mydev}: ${old_sum} -> ${sum}, diff=${diff1}"
    fi
done
echo total_diff = ${diff}
