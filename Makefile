IRON_SRC_PATHS = \
	src/iron/*.c \
	src/iron/opt/*.c \
	src/iron/xr17032/*.c \

SRCPATHS = \
	$(IRON_SRC_PATHS) \
	src/*.c \
	src/common/*.c \

SRC = $(wildcard $(SRCPATHS))
OBJECTS = $(SRC:src/%.c=build/%.o)

IRON_SRC = $(wildcard $(IRON_SRC_PATHS))
IRON_OBJECTS = $(IRON_SRC:src/%.c=build/%.o)

ECHO = echo

CC = gcc
LD = gcc

INCLUDEPATHS = -Isrc/
ASANFLAGS = -fsanitize=undefined -fsanitize=address
CFLAGS = -std=gnu2x -g -fwrapv -fno-strict-aliasing
WARNINGS = -Wall -Wimplicit-fallthrough -Wno-deprecated-declarations -Wno-enum-compare -Wno-unused -Wno-format -Wno-enum-conversion -Wincompatible-pointer-types -Wno-discarded-qualifiers -Wno-strict-aliasing
ALLFLAGS = $(CFLAGS) $(WARNINGS)
OPT = -Og

ifneq ($(OS),Windows_NT)
	CFLAGS += -rdynamic
	ECHO = /usr/bin/echo
	# JANK FIX FOR SANDWICH'S DUMB ECHO ON HIS LINUX MACHINE
endif

FILE_NUM = 0
build/%.o: src/%.c
	$(eval FILE_NUM=$(shell echo $$(($(FILE_NUM)+1))))
	$(shell $(ECHO) 1>&2 -e "\e[0m[\e[32m$(FILE_NUM)/$(words $(SRC))\e[0m]\t Compiling \e[1m$<\e[0m")
	
	@$(CC) -c -o $@ $< -MD $(INCLUDEPATHS) $(ALLFLAGS) $(OPT)

.PHONY: coyote
coyote: bin/coyote
bin/coyote: $(OBJECTS)
	@$(LD) $(OBJECTS) -o bin/coyote $(ALLFLAGS) -lm

.PHONY: iron
iron: bin/iron
bin/iron: bin/libiron.a src/iron/driver/driver.c
	@$(CC) src/iron/driver/driver.c -MD bin/libiron.a -o bin/iron $(INCLUDEPATHS) $(CFLAGS) $(OPT) -lm

bin/libiron.o: $(IRON_OBJECTS)
	@$(LD) $(IRON_OBJECTS) -r -o bin/libiron.o $(CFLAGS) -lm

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
	@mkdir -p $(dir $(OBJECTS))

-include $(OBJECTS:.o=.d)