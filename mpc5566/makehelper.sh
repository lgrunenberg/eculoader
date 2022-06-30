#!/bin/sh

SIZ3=$(wc -c out/mainloader.bin | awk '{print $1}')
SIZ2=$(wc -c out/mainloader.lz | awk '{print $1}')
SIZE=$(wc -c out/loader.bin | awk '{print $1}')

PRESIZE=$(expr $SIZE - $SIZ2)

echo "___Final blob sizes____________"
echo "Mainloader: $SIZ2 bytes (uncompressed $SIZ3 bytes)"
echo "Preloader : $PRESIZE bytes" 
echo "Total     : $SIZE bytes"
echo 
