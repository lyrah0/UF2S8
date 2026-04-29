#SHELL := /bin/fish
MAKEFLAGS := -j16 -l15
FLAGS_WARNINGS := -Wall -Wextra -Walloc-zero -Warray-bounds=2 -Wattribute-alias=2 -Wbad-function-cast -Wdate-time -Wdisabled-optimization -Wdouble-promotion -Wduplicated-branches -Wduplicated-cond -Wfloat-equal -Wformat=2 -Wformat-overflow=2 -Wformat-signedness -Wformat-truncation=2 -Wimplicit-fallthrough=5 -Winit-self -Winvalid-pch -Wlogical-op -Wmissing-include-dirs -Wnested-externs -Wnull-dereference -Wshadow=global -Wshift-overflow=2 -Wstrict-aliasing=3 -Wstrict-overflow=1 -Wstringop-overflow=4 -Wsuggest-attribute=pure -Wsuggest-attribute=const -Wsuggest-attribute=noreturn -Wsuggest-attribute=format -Wsuggest-attribute=cold -Wsuggest-attribute=malloc -Wsuggest-final-methods -Wsuggest-final-types -Wswitch-default -Wtrampolines -Wtrivial-auto-var-init -Wundef -Wunsuffixed-float-constants -Wuse-after-free=3 -Wvector-operation-performance -Woverlength-strings -Wpointer-arith -Wno-incompatible-pointer-types -Wno-pointer-sign -Wno-discarded-qualifiers

FLAGS_ANALYZER := -fanalyzer -fanalyzer-transitivity
FLAGS_DEBUG := -ggdb3

FLAGS_HARDENING :=-fstack-protector-strong -fstack-clash-protection
# Removed: -fsanitize-address-use-after-scope -fharden-compares -fharden-conditional-branches -fharden-control-flow-redundancy -fsanitize=undefined 
#FLAGS_SANITIZER := -fsanitize-recover=all -fsanitize=address,pointer-compare,pointer-subtract,leak,undefined -fsanitize-recover=all
#FLAGS_SANITIZER := -fsanitize-recover=all -fsanitize=address,pointer-compare,pointer-subtract,leak,undefined -fsanitize-recover=all

FLAGS_LTO := -fuse-linker-plugin -flto=auto -fdevirtualize-at-ltrans --param=lto-max-streaming-parallelism=16 --param=lto-partitions=16 -flto-compression-level=1

FLAGS_EXPERIMENTAL := -momit-leaf-frame-pointer -mrecip=all -mtls-dialect=gnu2 -ftree-vectorize -fvect-cost-model=very-cheap -fno-plt -fipa-pta -fdelete-null-pointer-checks -fmerge-all-constants -fno-var-tracking -ffinite-loops -fgcse-las -fgcse-sm -fweb

FLAGS_PARAMS_MAX := --param=ipa-sra-max-replacements=16 --param=max-cse-path-length=65536 --param=max-jump-thread-paths=65536 --param=max-sched-ready-insns=65536 --param=max-slsr-cand-scan=999999 --param=max-ssa-name-query-depth=10 --param=max-store-chains-to-track=65536 --param=max-stores-to-merge=65536 --param=max-stores-to-track=1048576 --param=modref-max-adjustments=254 --param=modref-max-depth=65536 --param=ranger-logical-depth=999 --param=ranger-recompute-depth=100 --param=relation-block-limit=9999 --param=uninit-control-dep-attempts=65536

FLAGS_PARAMS_X32 := --param=dse-max-alias-queries-per-store=8192 --param=dse-max-object-size=8192 --param=ipa-max-aa-steps=800000 --param=ipa-max-loop-predicates=512 --param=iv-consider-all-candidates-bound=1280 --param=iv-max-considered-uses=8000 --param=loop-max-datarefs-for-datadeps=32000 --param=lra-max-considered-reload-pseudos=16000 --param=max-crossjump-edges=3200 --param=max-cse-insns=32000 --param=max-cselib-memory-locations=16000 --param=max-dse-active-local-stores=160000 --param=max-hoist-depth=960 --param=max-isl-operations=11200000 --param=max-iterations-computation-cost=320 --param=max-iterations-to-track=32000 --param=max-last-value-rtl=320000 --param=max-reload-search-insns=3200 --param=max-speculative-devirt-maydefs=1600 --param=max-tail-merge-comparisons=320 --param=max-tail-merge-iterations=64 --param=max-tracked-strlens=320000 --param=modref-max-accesses=512 --param=modref-max-bases=1024 --param=modref-max-escape-points=8192 --param=modref-max-refs=512 --param=modref-max-tests=2048 --param=sccvn-max-alias-queries-per-access=32000 --param=scev-max-expr-complexity=320 --param=scev-max-expr-size=3200 --param=sra-max-propagations=1024 --param=ssa-name-def-chain-limit=16384 --param=ira-max-conflict-table-size=32000 --param=loop-invariant-max-bbs-in-loop=320000 --param=max-gcse-memory=4194304 --param=max-partial-antic-length=3200 --param=max-pending-list-length=1024

GCC_CFLAGS := -pipe -std=gnu23 -g0 -Ofast -march=native $(FLAGS_WARNINGS) $(FLAGS_ANALYZER) $(FLAGS_HARDENING) $(FLAGS_SANITIZER) $(FLAGS_LTO) $(FLAGS_PARAMS_MAX) $(FLAGS_PARAMS_X32) --param=ggc-min-heapsize=1048576 --param=ggc-min-expand=10 -ftrack-macro-expansion=0 $(FLAGS_EXPERIMENTAL)
# #-ggdb3 

CFLAGS := $(GCC_CFLAGS)

AVX512_IS_KILL := -mno-avx512f -mno-avx512pf -mno-avx512er -mno-avx512cd -mno-avx512vl -mno-avx512bw -mno-avx512dq -mno-avx512ifma -mno-avx512vbmi -mno-avx512vbmi2 -mno-avx512bf16 -mno-avx512fp16 -mno-avx512bitalg -mno-avx512vpopcntdq -mno-avx512vp2intersect -mno-avx5124fmaps -mno-avx512vnni -mno-avx5124vnniw -mno-evex512

# -Wno-analyzer-malloc-leak -Wno-analyzer-possible-null-argument
# -Wno-analyzer-use-of-uninitialized-value
LDFLAGS :=-fuse-ld=mold -Wl,-O2 -Wl,--as-needed -Wl,--hash-style=gnu -Wl,-z,relro -Wl,-z,now -Wl,--gc-sections -Wl,-z,combreloc -no-pie -Wl,-z,pack-relative-relocs -Wl,--icf=all -Wl,--ignore-data-address-equality
CPPFLAGS :=-D_GLIBCXX_ASSERTIONS -D_FORTIFY_SOURCE=3 -DNDEBUG

#MISCFLAGS := -Isrc -rdynamic $(OPTFLAGS)
MISCFLAGS := -Isrc $(OPTFLAGS)
# LIBS := -ldl $(OPTLIBS)
LIBS := $(OPTLIBS) -lbsd -lbstring -lc

CC := ccache gcc
AR := gcc-ar

debug ?= 0
valgrind ?= 0

SRC_DIR := ./src
BUILD_DIR := ./build
TESTS_DIR := ./tests
BIN_DIR := ./bin

HDR_FILES := $(wildcard $(SRC_DIR)/**/*.h $(SRC_DIR)/*.h)
SRC_FILES := $(wildcard $(SRC_DIR)/**/*.c $(SRC_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%, $(BUILD_DIR)/%, $(patsubst %.c,%.o,$(SRC_FILES)))
OBJ_FILES_LIB := $(filter-out $(SRC_DIR)/main.c, $(OBJ_FILES))

DEP_FILES := $(patsubst $(SRC_DIR)/%, $(BUILD_DIR)/%, $(patsubst %.c,%.d,$(SRC_FILES)))

SRC_TESTS := $(wildcard $(TESTS_DIR)/*.c)
EXEC_TESTS := $(patsubst %.c, %, $(SRC_TESTS))

TARGET := emulator
TARGET_EXEC := $(BIN_DIR)/$(TARGET)
TARGET_LIB_STATIC := $(BIN_DIR)/lib$(TARGET).a
TARGET_LIB_SHARED :=  $(BIN_DIR)/lib$(TARGET).so


ifeq ($(debug),1)
	CFLAGS := $(filter-out -g0, $(CFLAGS)) -ggdb3
	CPPFLAGS := $(filter-out -DNDEBUG, $(CPPFLAGS))
	LDFLAGS := $(filter-out -s, $(LDFLAGS))
endif

ifeq ($(valgrind),1)
	valgrind := valgrind
	CFLAGS := $(filter-out $(FLAGS_SANITIZER), $(CFLAGS))
else
	valgrind := 
endif

.PHONY: make_dirs all clean check target debug tests cdebug cdebugv exportv

all: make_dirs $(TARGET_EXEC) check

make_dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(BUILD_DIR)

tests: $(EXEC_TESTS)
	./tests/runtests.sh $(valgrind)

debug0: CPPFLAGS := $(CPPFLAGS) -DDEBUG0
debug0: all


# Build targets
$(TARGET_EXEC): $(OBJ_FILES) | make_dirs
	$(CC) $(LIBS) $(CPPFLAGS) $(CFLAGS) $(MISCFLAGS) $(LDFLAGS) $^ -o $@

build_libs: $(TARGET_LIB_STATIC) $(TARGET_LIB_SHARED)

$(TARGET_LIB_STATIC): $(OBJ_FILES_LIB)
	$(AR) rcs $@ $^
	ranlib $@

$(TARGET_LIB_SHARED): CFLAGS := $(filter-out -fno-pie, $(CFLAGS)) -fPIC
$(TARGET_LIB_SHARED): LDFLAGS := $(filter-out -no-pie -s, $(LDFLAGS))
$(TARGET_LIB_SHARED): $(OBJ_FILES_LIB)
	$(CC) -shared $(LIBS) $(CPPFLAGS) $(CFLAGS) $(MISCFLAGS) $(LDFLAGS) $^ -o $@

# Build tests
$(EXEC_TESTS): %: %.c $(TARGET_LIB_STATIC) $(OBJ_FILES_LIB)
	$(CC) $(LIBS) $(CPPFLAGS) $(CFLAGS) $(MISCFLAGS) $(LDFLAGS) $< $(TARGET_LIB_STATIC)  -o $@

-include $(DEP_FILES)

# Automatic object files
#%.o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c Makefile
	# $(LDFLAGS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MISCFLAGS) -MMD -MP -c $< -o $@

clean:
	-rm tests/tests.log $(OBJ_FILES) $(EXEC_TESTS) $(TARGET_EXEC) $(TARGET_LIB_STATIC) $(TARGET_LIB_SHARED) $(wildcard *.o) $(wildcard *.i) $(wildcard *.s)

check: $(HDR_FILES) $(SRC_FILES) $(SRC_TESTS)
	ctags -R
	clang-format -i $?
	run-clang-tidy -j 16 -fix -use-color 1 $?
#	@echo Files with potentially dangerous functions.
#	@grep -E '[^_.>a-zA-Z0-9](str(n?cpy|n?cat|xfrm|n?dup|str|pbrk|tok|_)|stpn?cpy|a?sn?printf|byte_)' $? || true
