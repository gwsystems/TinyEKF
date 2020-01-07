#
# Makefile for TinyEKF GPS example
#
# Copyright (C) 2015 Simon D. Levy
#
# MIT License

WCC=/opt/wasi-sdk/bin/clang
WSYSROOT=/opt/wasi-sdk/share/wasi-sysroot/
WCFLAGS += --target=wasm32-wasi


SRC = ../../src

all: gps_ekf gps_ekf.wasi

run: gps_ekf
	./gps_ekf


gps_ekf.em: gps_ekf.c $(SRC)/tiny_ekf.c $(SRC)/tiny_ekf.h
	emcc -s ALLOW_MEMORY_GROWTH=1 -O3 -g -Wall -I. -I$(SRC) -o gps_ekf.em.js gps_ekf.c $(SRC)/tiny_ekf.c -lm

gps_ekf: gps_ekf.c $(SRC)/tiny_ekf.c $(SRC)/tiny_ekf.h
	clang -O3 -g -Wall -I. -I$(SRC) -o gps_ekf gps_ekf.c $(SRC)/tiny_ekf.c -lm
	gcc -O3 -g -Wall -I. -I$(SRC) -o gps_ekf.x gps_ekf.c $(SRC)/tiny_ekf.c -lm

gps_ekf.wasi: gps_ekf.c $(SRC)/tiny_ekf.c $(SRC)/tiny_ekf.h
	${WCC} -O3 -g -Wall -I. -I$(SRC) -o gps_ekf.wasi ${WCFLAGS} gps_ekf.c $(SRC)/tiny_ekf.c -lm -Wl,--allow-undefined,-z,stack-size=524288,--no-threads,--stack-first,--no-entry,--export-all,--export=main --sysroot=${WSYSROOT} -flto

edit:
	vim gps_ekf.c

clean:
	rm -f gps_ekf gps_ekf.x gps_ekf.wasi *.o *~ ekf.csv
