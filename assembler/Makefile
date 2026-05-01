#SHELL := /bin/fish
NPROC= $(shell nproc)
MAKEFLAGS := -j$(NPROC)
FLAGS_WARNINGS := -Wall -Wextra -Walloc-zero -Warray-bounds=2 -Wattribute-alias=2 -Wbad-function-cast -Wdate-time -Wdisabled-optimization -Wdouble-promotion -Wduplicated-branches -Wduplicated-cond -Wfloat-equal -Wformat=2 -Wformat-overflow=2 -Wformat-signedness -Wformat-truncation=2 -Wimplicit-fallthrough=5 -Winit-self -Winvalid-pch -Wlogical-op -Wmissing-include-dirs -Wnested-externs -Wnull-dereference -Wshadow=global -Wshift-overflow=2 -Wstrict-aliasing=3 -Wstrict-overflow=1 -Wstringop-overflow=4 -Wsuggest-attribute=pure -Wsuggest-attribute=const -Wsuggest-attribute=noreturn -Wsuggest-attribute=format -Wsuggest-attribute=cold -Wsuggest-attribute=malloc -Wsuggest-final-methods -Wsuggest-final-types -Wswitch-default -Wtrampolines -Wtrivial-auto-var-init -Wundef -Wunsuffixed-float-constants -Wuse-after-free=3 -Wvector-operation-performance -Woverlength-strings -Wpointer-arith -Wno-incompatible-pointer-types -Wno-pointer-sign -Wno-discarded-qualifiers

FLAGS_ANALYZER := -fanalyzer -fanalyzer-transitivity
FLAGS_DEBUG := -ggdb3

FLAGS_HARDENING :=-fstack-protector-strong -fstack-clash-protection
# Removed: -fsanitize-address-use-after-scope -fharden-compares -fharden-conditional-branches -fharden-control-flow-redundancy -fsanitize=undefined 
#FLAGS_SANITIZER := -fsanitize-recover=all -fsanitize=address,pointer-compare,pointer-subtract,leak,undefined -fsanitize-recover=all

FLAGS_LTO := -fuse-linker-plugin -flto=auto -fdevirtualize-at-ltrans --param=lto-max-streaming-parallelism=$(NPROC) --param=lto-partitions=$(NPROC)

FLAGS_EXPERIMENTAL := -momit-leaf-frame-pointer -mrecip=all -mtls-dialect=gnu2 -ftree-vectorize -fno-plt -fipa-pta -fdelete-null-pointer-checks -fmerge-all-constants -fno-var-tracking -ffinite-loops -fweb


GCC_CFLAGS := -pipe -std=gnu23 -g0 -Ofast -march=native $(FLAGS_WARNINGS) $(FLAGS_ANALYZER) $(FLAGS_HARDENING) $(FLAGS_SANITIZER) $(FLAGS_LTO) --param=ggc-min-expand=10 -ftrack-macro-expansion=0 $(FLAGS_EXPERIMENTAL)

CFLAGS := $(GCC_CFLAGS)

LDFLAGS :=-fuse-ld=mold -Wl,-O2 -Wl,--as-needed -Wl,--hash-style=gnu -Wl,-z,relro -Wl,-z,now -Wl,--gc-sections -Wl,-z,combreloc -no-pie -Wl,-z,pack-relative-relocs -Wl,--icf=all -Wl,--ignore-data-address-equality
CPPFLAGS :=-D_GLIBCXX_ASSERTIONS -D_FORTIFY_SOURCE=3 -DNDEBUG

MISCFLAGS := -Isrc $(OPTFLAGS) $(shell pkg-config --cflags sdl3)
LIBS := $(OPTLIBS) -lbsd -lbstring -lc $(shell pkg-config --libs sdl3)

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

TARGET := assembler
TARGET_EXEC := $(BIN_DIR)/$(TARGET)
TARGET_LIB_STATIC := $(BIN_DIR)/lib$(TARGET).a
TARGET_LIB_SHARED :=  $(BIN_DIR)/lib$(TARGET).so


ifeq ($(debug),1)
	CFLAGS := $(filter-out -g0, $(CFLAGS)) $(FLAGS_DEBUG)
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
	run-clang-tidy -j $(NPROC) -fix -use-color 1 $?
