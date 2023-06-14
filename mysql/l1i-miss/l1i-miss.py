import math
import random
import re

SET = 256
WAY = 4
CL = 64

ref_count = 0
miss_count = 0

cl_shift = int(math.log2(CL))
assert (1 << cl_shift) == CL

addr_regex = re.compile('^[0-9a-f]{8,16}$')

cl_sets = [[0]*WAY for _ in range(SET)]
lfu_list = [[0]*WAY for _ in range(SET)]
lfu_age_list = [[0]*WAY for _ in range(SET)]

def insert_cl_random(addr):
    global miss_count

    addr = int(addr, 16)
    addr &= ~(CL - 1)
    cl_index = (addr >> cl_shift) & (SET - 1)
    cl_set = cl_sets[cl_index]
    for i in range(WAY):
        if cl_set[i] == addr:
            return
    miss_count += 1
    cl_set[random.randint(0,3)] = addr

def insert_cl_lru(addr):
    global miss_count

    addr = int(addr, 16)
    addr &= ~(CL - 1)
    cl_index = (addr >> cl_shift) & (SET - 1)
    cl_set = cl_sets[cl_index]
    for i in range(WAY):
        if cl_set[i] == addr:
            # move to head
            for j in range(i-1, -1, -1):
                cl_set[j+1] = cl_set[j]
            cl_set[0] = addr
            return
    miss_count += 1
    for i in range(WAY-2, -1, -1):
        cl_set[i+1] = cl_set[i]
    cl_set[0] = addr

def insert_cl_lfu(addr):
    global miss_count

    addr = int(addr, 16)
    addr &= ~(CL - 1)
    cl_index = (addr >> cl_shift) & (SET - 1)
    cl_set = cl_sets[cl_index]
    lfu = lfu_list[cl_index]
    for i in range(WAY):
        if cl_set[i] == addr:
            lfu[i] += 1
            return  # hit
    miss_count += 1
    for i in range(WAY):
        if cl_set[i] == 0:
            cl_set[i] = addr
            lfu[i] = 1
            return

    idx = 0
    for i in range(1, WAY):
        if lfu[i] < lfu[idx]:
            idx = i
    cl_set[idx] = addr
    lfu[idx] = 1

def insert_cl_lfuda(addr):
    def get_cost(cnt, age):
        return 3*cnt - age

    global miss_count

    addr = int(addr, 16)
    addr &= ~(CL - 1)
    cl_index = (addr >> cl_shift) & (SET - 1)
    cl_set = cl_sets[cl_index]
    lfu = lfu_list[cl_index]
    lfu_age = lfu_age_list[cl_index]
    for i in range(WAY):
        if cl_set[i] == addr:
            lfu[i] += 1
            lfu_age[i] = 0
            return  # hit
    miss_count += 1
    for i in range(WAY):
        if cl_set[i] == 0:
            cl_set[i] = addr
            lfu[i] = 1
            lfu_age[i] = 0
            return

    cost = [0]*WAY
    for i in range(WAY):
        cnt = lfu[i]
        age = lfu_age[i]
        cost[i] = get_cost(cnt, age)
        lfu_age[i] += 1

    # find smallest cost
    idx = random.randint(0, WAY-1)
    for i in range(WAY):
        if cost[i] < cost[idx]:
            idx = i

    cl_set[idx] = addr
    lfu[idx] = 1
    lfu_age[idx] = 0

def parse(line):
    global ref_count

    addr = line.split()[5]
    if not addr_regex.match(addr):
        raise Exception(f'invalud addr: {addr}')
    ref_count += 1
    insert_cl_lru(addr)

with open('l1i-perf.script', 'r') as f:
    for line in f:
        if not 'kernel.kallsyms' in line:
            parse(line)

print(f'l1i miss ratio = {miss_count*100/ref_count}%')
