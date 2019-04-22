all: encode-origin encode-quick

encode-origin: ceph-utf8.c
	gcc -O3 $^ -o $@

encode-quick: ceph-utf8.c
	gcc -O3 -DBENCH_QUICK $^ -o $@

clean:
	rm -f encode-origin encode-quick
