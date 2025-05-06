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
CFLAGS = -std=gnu2x -g -Wall -Wimplicit-fallthrough -fwrapv -Wno-enum-compare -Wno-unused -Wno-format -Wno-enum-conversion -Wincompatible-pointer-types -Wno-discarded-qualifiers -lm -Wno-deprecated-declarations
OPT = -O0

ifneq ($(OS),Windows_NT)
	CFLAGS += -rdynamic
	ECHO = /usr/bin/echo
	# JANK FIX FOR SANDWICH'S DUMB ECHO ON HIS LINUX MACHINE
endif

FILE_NUM = 0
build/%.o: src/%.c
	$(eval FILE_NUM=$(shell echo $$(($(FILE_NUM)+1))))
	$(shell $(ECHO) 1>&2 -e "\e[0m[\e[32m$(FILE_NUM)/$(words $(SRC))\e[0m]\t Compiling \e[1m$<\e[0m")
	
	@$(CC) -c -o $@ $< -MD $(INCLUDEPATHS) $(CFLAGS) $(OPT)

coyote: $(OBJECTS)
	@$(LD) $(OBJECTS) -o coyote $(CFLAGS)

iron: libiron.a src/iron/driver/driver.c
	@$(CC) src/iron/driver/driver.c libiron.a -o iron  $(INCLUDEPATHS) $(CFLAGS) $(OPT)
	
libiron.o: $(IRON_OBJECTS)
	@$(LD) $(IRON_OBJECTS) -r -o libiron.o $(CFLAGS)

libiron.a: libiron.o
	@ar rcs libiron.a libiron.o


clean:
	@rm -rf build/
	@mkdir build/
	@mkdir -p $(dir $(OBJECTS))

-include $(OBJECTS:.o=.d)