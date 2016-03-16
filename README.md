# hzip
Compressing files using huffman coding.


## compile
`gcc hzip.c -std=c99 -O2 -s -o hzip`

## usage
Run `./hzip -h` to see usage.
```
Usage: hzip [OPTION]... [-o OUTFILE] [FILE]
       hzip -d [OPTION]... [-o OUTFILE] [FILE]
Encode or decode FILE using huffman coding.

Available options are:
    -d, --decode               decode FILE.
    -e, -c, --encode           encode FILE, this option is not nessasary.
    -o, --outfile OUTFILE      write output to OUTFILE.
    -v, --verbose              verbose mode.
    -q, --quiet                suppress error messages.
    -t, --dumptree             show huffman tree.
    -a, --dumptable            show huffman codes.
    -n, --count                count the occurence of every byte.
    -s, --showstat             show statistic infmation such as ratio.
    -h, -?, --help             give this help.
    -V, --version              show version.

Default FILE and OUTFILE is "-" (stdin or stdout).

hzip v0.1
```

## benchmark
Run `./benchmark.sh` to see the speed of encoding.

Below is the benchmark on my system.
```
benchmarking './hzip' on '/dev/zero'
1+0 records in
1+0 records out
20971520 bytes (21 MB) copied, 0.779633 s, 26.9 MB/s
benchmarking './hzip' on '/dev/urandom'
1+0 records in
1+0 records out
20971520 bytes (21 MB) copied, 1.09061 s, 19.2 MB/s

benchmarking 'gzip' on '/dev/zero'
1+0 records in
1+0 records out
20971520 bytes (21 MB) copied, 0.360888 s, 58.1 MB/s
benchmarking 'gzip' on '/dev/urandom'
1+0 records in
1+0 records out
20971520 bytes (21 MB) copied, 2.06924 s, 10.1 MB/s

benchmarking 'gzip -1' on '/dev/zero'
1+0 records in
1+0 records out
20971520 bytes (21 MB) copied, 0.292853 s, 71.6 MB/s
benchmarking 'gzip -1' on '/dev/urandom'
1+0 records in
1+0 records out
20971520 bytes (21 MB) copied, 2.03918 s, 10.3 MB/s

benchmarking 'bzip2' on '/dev/zero'
1+0 records in
1+0 records out
20971520 bytes (21 MB) copied, 0.335834 s, 62.4 MB/s
benchmarking 'bzip2' on '/dev/urandom'
1+0 records in
1+0 records out
20971520 bytes (21 MB) copied, 9.48297 s, 2.2 MB/s

benchmarking 'xz' on '/dev/zero'
1+0 records in
1+0 records out
20971520 bytes (21 MB) copied, 1.63233 s, 12.8 MB/s
benchmarking 'xz' on '/dev/urandom'
1+0 records in
1+0 records out
20971520 bytes (21 MB) copied, 16.1024 s, 1.3 MB/s

benchmarking 'compress' on '/dev/zero'
1+0 records in
1+0 records out
20971520 bytes (21 MB) copied, 0.241079 s, 87.0 MB/s
benchmarking 'compress' on '/dev/urandom'
1+0 records in
1+0 records out
20971520 bytes (21 MB) copied, 1.409 s, 14.9 MB/s
```
The performance of encoding in worse case is better than gzip, bzip2, xz.

## file structure
TBA
