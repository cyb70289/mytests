all: encode-origin encode-quick bench-inline

encode-origin: ceph-utf8.c
	$(CC) -O3 $^ -o $@

encode-quick: ceph-utf8.c
	$(CC) -O3 -DBENCH_QUICK $^ -o $@

bench-inline: bench-inline.c
	$(CC) -O3 $^ -o $@

clean:
	rm -f encode-origin encode-quick bench-inline
