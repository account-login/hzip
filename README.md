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
