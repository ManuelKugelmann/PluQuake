#!/bin/sh

# Cross-compile for Windows x64 using LLVM-MinGW (UCRT)
# LLVM-MinGW is required because nng 1.11 requires UCRT runtime

TARGET=x86_64-w64-mingw32
PREFIX=/opt/llvm-mingw/llvm-mingw-20251104-ucrt-ubuntu-22.04-x86_64

PATH="$PREFIX/bin:$PATH"
export PATH

MAKE_CMD=make

CC="$TARGET-gcc"
AS="$TARGET-as"
RANLIB="$TARGET-ranlib"
AR="$TARGET-ar"
WINDRES="$TARGET-windres"
STRIP="$TARGET-strip"
export PATH CC AS AR RANLIB WINDRES STRIP

exec $MAKE_CMD CC=$CC AS=$AS RANLIB=$RANLIB AR=$AR WINDRES=$WINDRES STRIP=$STRIP -f Makefile.w64 $*
