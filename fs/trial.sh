#!/bin/bash
/opt/cc65/bin/cc65 test6502.c
/opt/cc65/bin/ca65 test6502.s
/opt/cc65/bin/cl65 test6502.o

ls -l test6502.*


