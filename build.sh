#!/bin/sh

avrdude -c usbasp -p t84 -U lfuse:w:0xe2:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m

make clean && make && make program
