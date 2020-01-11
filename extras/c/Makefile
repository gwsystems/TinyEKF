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

all: gps_ekf_fn #gps_ekf_fn.wasi

run: gps_ekf_fn
	./gps_ekf_fn


gps_ekf_fn.em: gps_ekf_fn.c $(SRC)/tiny_ekf.c $(SRC)/tiny_ekf.h
	emcc -s ALLOW_MEMORY_GROWTH=1 -O3 -g -Wall -I. -I$(SRC) -o gps_ekf_fn.em.js gps_ekf_fn.c $(SRC)/tiny_ekf.c -lm

gps_ekf_fn: gps_ekf_fn.c $(SRC)/tiny_ekf.c $(SRC)/tiny_ekf.h
	clang -O3 -g -I. -I$(SRC) -o gps_ekf_fn gps_ekf_fn.c $(SRC)/tiny_ekf.c -lm
	gcc -O3 -g -I. -I$(SRC) -o gps_ekf_fn.x gps_ekf_fn.c $(SRC)/tiny_ekf.c -lm

gps_ekf_fn.wasi: gps_ekf_fn.c $(SRC)/tiny_ekf.c $(SRC)/tiny_ekf.h
	${WCC} -O3 -g -Wall -I. -I$(SRC) -o gps_ekf_fn.wasi ${WCFLAGS} gps_ekf_fn.c $(SRC)/tiny_ekf.c -lm -Wl,--allow-undefined,-z,stack-size=524288,--no-threads,--stack-first,--no-entry,--export-all,--export=main --sysroot=${WSYSROOT} -flto

edit:
	vim gps_ekf_fn.c

clean:
	rm -f gps_ekf_fn gps_ekf_fn.x gps_ekf_fn.wasi *.o *~ ekf.csv
