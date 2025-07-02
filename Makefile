IRON_SRC_PATHS = \
	src/iron/*.c \
	src/iron/opt/*.c \
	src/iron/xr17032/*.c \

COYOTE_SRC_PATHS = \
	src/coyote/*.c \
	src/common/*.c \

COBALT_SRC_PATHS = \
	src/cobalt/*.c \
	src/common/*.c \

IRON_SRC = $(wildcard $(IRON_SRC_PATHS))
IRON_OBJECTS = $(IRON_SRC:src/%.c=build/%.o)

COYOTE_SRC = $(wildcard $(COYOTE_SRC_PATHS))
COYOTE_OBJECTS = $(COYOTE_SRC:src/%.c=build/%.o)

COBALT_SRC = $(wildcard $(COBALT_SRC_PATHS))
COBALT_OBJECTS = $(COBALT_SRC:src/%.c=build/%.o)

CC = gcc
LD = gcc

INCLUDEPATHS = -Isrc/
ASANFLAGS = -fsanitize=undefined -fsanitize=address
CFLAGS = -std=gnu2x -g0 -fwrapv -fno-strict-aliasing
WARNINGS = -Wall -Wimplicit-fallthrough -Wno-deprecated-declarations -Wno-enum-compare -Wno-unused -Wno-format -Wno-enum-conversion -Wincompatible-pointer-types -Wno-discarded-qualifiers -Wno-strict-aliasing
ALLFLAGS = $(CFLAGS) $(WARNINGS)
OPT = -O3

ifneq ($(OS),Windows_NT)
	CFLAGS += -rdynamic
endif

FILE_NUM = 0
build/%.o: src/%.c
	$(eval FILE_NUM=$(shell echo $$(($(FILE_NUM)+1))))
	$(shell echo 1>&2 -e "Compiling \e[1m$<\e[0m")
	
	@$(CC) -c -o $@ $< -MD $(INCLUDEPATHS) $(ALLFLAGS) $(OPT)

.PHONY: coyote
coyote: bin/coyote
bin/coyote: bin/libiron.a $(COYOTE_OBJECTS)
	@$(LD)  $(COYOTE_OBJECTS) -o bin/coyote -Lbin -lm -liron

.PHONY: cobalt
cobalt: bin/cobalt
bin/cobalt: bin/libiron.a $(COBALT_OBJECTS)
	@$(LD) $(COBALT_OBJECTS) -o bin/cobalt -Lbin -lm -liron

.PHONY: iron
iron-test: bin/iron-test
bin/iron-test: bin/libiron.a src/iron/driver/driver.c
	@$(CC) src/iron/driver/driver.c -o bin/iron-test $(INCLUDEPATHS) $(CFLAGS) $(OPT) -Lbin -lm -liron

bin/libiron.o: $(IRON_OBJECTS)
	@$(LD) $(IRON_OBJECTS) -r -o bin/libiron.o -lm

.PHONY: libiron
libiron: bin/libiron.a
bin/libiron.a: bin/libiron.o
	@ar rcs bin/libiron.a bin/libiron.o

.PHONY: clean
clean:
	@rm -rf build/
	@rm -rf bin/
	@mkdir build/
	@mkdir bin/
	@mkdir -p $(dir $(IRON_OBJECTS))
	@mkdir -p $(dir $(COYOTE_OBJECTS))
	@mkdir -p $(dir $(COBALT_OBJECTS))

-include $(IRON_OBJECTS:.o=.d)
-include $(COYOTE_OBJECTS:.o=.d)
-include $(COBALT_OBJECTS:.o=.d)
