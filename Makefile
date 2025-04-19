SRCPATHS = \
	src/*.c \
	src/iron/*.c \
	src/iron/opt/*.c \
	src/iron/xr17032/*.c \
	src/common/*.c \

SRC = $(wildcard $(SRCPATHS))
OBJECTS = $(SRC:src/%.c=build/%.o)

EXECUTABLE_NAME = coyote
ECHO = echo

CC = gcc
LD = gcc

INCLUDEPATHS = -Isrc/
DEBUGFLAGS = -lm -pg -g -Wall -Wextra 
ASANFLAGS = -fsanitize=undefined -fsanitize=address
CFLAGS = -g -Wall -Wimplicit-fallthrough -fwrapv -Wno-format -MD -Wincompatible-pointer-types -Wno-discarded-qualifiers -lm -Wno-deprecated-declarations -Wno-switch
OPT = -O0

ifeq ($(OS),Windows_NT)
	EXECUTABLE_NAME = coyote.exe
else
	CFLAGS += -rdynamic
	ECHO = /usr/bin/echo
	# JANK FIX FOR SANDWICH'S DUMB ECHO ON HIS LINUX MACHINE
endif


FILE_NUM = 0
build/%.o: src/%.c
	$(eval FILE_NUM=$(shell echo $$(($(FILE_NUM)+1))))
	$(shell $(ECHO) 1>&2 -e "\e[0m[\e[32m$(FILE_NUM)/$(words $(SRC))\e[0m]\t Compiling \e[1m$<\e[0m")
	@$(CC) -c -o $@ $< $(INCLUDEPATHS) $(CFLAGS) $(OPT)

build: $(OBJECTS)
	@$(LD) $(OBJECTS) -o $(EXECUTABLE_NAME) $(CFLAGS)

debug: CFLAGS += $(DEBUGFLAGS)
debug: OPT = -Og
debug: build

clean:
	@rm -rf build/
	@mkdir build/
	@mkdir -p $(dir $(OBJECTS))

cleanbuild: clean build

-include $(OBJECTS:.o=.d)
