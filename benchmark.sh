#!/bin/bash

benchmark()
{
	FILE="$1"
	shift
	APP="$@"
	
	echo "benchmarking '$APP' on '$FILE'"
	dd if="$FILE" bs=20M count=1 | "$@" >/dev/null
}

benchmark /dev/zero ./hzip
benchmark /dev/urandom ./hzip
echo

if [ -x ./hzip.orig ]; then
	benchmark /dev/zero ./hzip.orig
	benchmark /dev/urandom ./hzip.orig
	echo
fi

benchmark /dev/zero gzip
benchmark /dev/urandom gzip
echo

benchmark /dev/zero gzip -1
benchmark /dev/urandom gzip -1
echo

benchmark /dev/zero bzip2
benchmark /dev/urandom bzip2
echo

benchmark /dev/zero xz
benchmark /dev/urandom xz
echo

benchmark /dev/zero compress
benchmark /dev/urandom compress
echo
