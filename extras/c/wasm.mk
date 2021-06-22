BASE_DIR=../../../../../

AWSM_CC=awsm

WASM_CC=wasm32-unknown-unknown-wasm-clang
WASMLINKFLAGS=-Wl,-z,stack-size=524288,--allow-undefined,--no-threads,--stack-first,--no-entry,--export-all,--export=main,--export=dummy

# The SLEDGE, WASM, and CPU_FREQ defines trigger preprocessor conditionals in the TinyEKF source code
# It looks like this has to set when compiling the WebAssembly module.
WASMCFLAGS_OUT=${WASMLINKFLAGS} -nostartfiles -DWASM -DCPU_FREQ=3600 -g -I. -I../../src $(LDFLAGS) -lm
WASMCFLAGS_SO=${WASMCFLAGS_OUT} -DSLEDGE

NATIVE_CC=clang
NATIVE_CC_CFLAGS=-DUSE_MEM_VM # to be used after awsm_cc generates *.bc file
LDFLAGS=-lm
NATIVE_CFLAGS=-g -I. -I../../src $(LDFLAGS) # to be used for native binary

# Used with both NATIVE_CC and WASM_CC
# -w = supress all warnings... We don't want to deviate from upstream source, so we won't fix anyways
OPTFLAGS=-O3 -flto -w

MEMC_64=64bit_nix.c

# for aWsm compiler
# Currently only uses wasmception backing
AWSM_DIR=${BASE_DIR}/awsm/
AWSM_RT_DIR=${AWSM_DIR}/runtime/
AWSM_RT_MEM=${AWSM_RT_DIR}/memory/
AWSM_RT_LIBC=${AWSM_RT_DIR}/libc/wasmception_backing.c
AWSM_RT_ENV=${AWSM_RT_DIR}/libc/env.c
AWSM_RT_RT=${AWSM_RT_DIR}/runtime.c
AWSM_RT_MEMC=${AWSM_RT_MEM}/${MEMC_64}
DUMMY=${AWSM_DIR}/code_benches/dummy.c

# for SLEdge serverless runtime
SLEDGE_RT_DIR=${BASE_DIR}/runtime/
SLEDGE_RT_INC=${SLEDGE_RT_DIR}/include/
SLEDGE_MEMC=${SLEDGE_RT_DIR}/compiletime/memory/${MEMC_64}
WASMISA=${SLEDGE_RT_DIR}/compiletime/instr.c

# WASI_SDK Stuff
#EXTRA_FLAGS=-lm
#WCC=/opt/wasi-sdk/bin/clang
#WSYSROOT=/opt/wasi-sdk/share/wasi-sysroot/
#WCFLAGS += --target=wasm32-wasi

SRC=../../src/tiny_ekf.c

all: gps_ekf_fn gps_ekf_fn.out gps_ekf_fn.so

gps_ekf_fn: $(SRC) gps_ekf_fn.c
	$(NATIVE_CC) $(NATIVE_CFLAGS) ${OPTFLAGS} gps_ekf_fn.c ${SRC} -o gps_ekf_fn

# Compile source into a WebAssembly Module. Unfortunately, this has to be recompiled targeting either aWsm or SLEdge
gps_ekf_fn_out.wasm: $(SRC) gps_ekf_fn.c
	@${WASM_CC} ${WASMCFLAGS_OUT} ${OPTFLAGS} gps_ekf_fn.c ${SRC} ${DUMMY} -o gps_ekf_fn_out.wasm

gps_ekf_fn_so.wasm: $(SRC) gps_ekf_fn.c
	@${WASM_CC} ${WASMCFLAGS_SO} ${OPTFLAGS} gps_ekf_fn.c ${SRC} ${DUMMY} -o gps_ekf_fn_so.wasm


# Compile the WebAssembly Module for direct execution as an ELF binary
gps_ekf_fn.out: gps_ekf_fn_out.wasm
	@${AWSM_CC} gps_ekf_fn_out.wasm -o gps_ekf_fn_out.bc
	@${NATIVE_CC} ${NATIVE_CC_CFLAGS} ${OPTFLAGS} gps_ekf_fn_out.bc ${AWSM_RT_MEMC} ${AWSM_RT_LIBC} ${AWSM_RT_ENV} ${AWSM_RT_RT} -o gps_ekf_fn.out

# Compile the WebAssembly Module into a SLEdge serverless *.so module
gps_ekf_fn.so: gps_ekf_fn_so.wasm
	@${AWSM_CC} --inline-constant-globals --runtime-globals gps_ekf_fn_so.wasm -o gps_ekf_fn_so.bc
	@${NATIVE_CC} --shared -fPIC ${OPTFLAGS} -I${SLEDGE_RT_INC} ${NATIVE_CC_CFLAGS} gps_ekf_fn_so.bc ${SLEDGE_MEMC} ${WASMISA} -o gps_ekf_fn.so

# An early attempt at WASI support
# gps_ekf_fn.wasi: $(SRC) gps_ekf_fn.c
# 	${WCC} -o gps_ekf_fn.wasi gps_ekf_fn.c ${SRC} ${WCFLAGS} ${EXTRA_CFLAGS} -Wl,--allow-undefined,-z,stack-size=524288,--no-threads,--stack-first,--no-entry,--export-all,--export=main --sysroot=${WSYSROOT} -O3 -flto

# A template for how to build using aWsm
# hello.sf: hello.c
# 	${WASM_CC} ${WASMCFLAGS} ${EXTRA_CFLAGS} ${OPTFLAGS} hello.c ${DUMMY} -o hello.wasm
# 	${AWSM_CC} hello.wasm -o hello.bc
# 	${NATIVE_CC} ${EXTRA_CFLAGS} ${OPTFLAGS} -D${USE_MEM} hello.bc ${MEMC} ${RT_LIBC} ${RT_RT} -o hello.out
# 	${AWSM_CC} --inline-constant-globals --runtime-globals hello.wasm -o hello.bc
# 	${NATIVE_CC} --shared -fPIC ${OPTFLAGS} -D${USE_MEM} -I${ART_INC} ${EXTRA_CFLAGS} hello.bc ${AMEMC} ${WASMISA} -o hello.aso

clean:
	@rm -f *.wasm *.bc *.out *.so gps_ekf_fn

