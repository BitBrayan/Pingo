CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -O2 $(shell sdl2-config --cflags)
MIXER_LIBS := $(shell pkg-config --libs SDL2_mixer 2>/dev/null)
ifneq ($(MIXER_LIBS),)
LDFLAGS = $(shell sdl2-config --libs) -lSDL2_ttf $(MIXER_LIBS) -lm
else
LDFLAGS = $(shell sdl2-config --libs) -lSDL2_ttf -lm
endif
SRC     = $(wildcard *.c)
OBJ     = $(SRC:.c=.o)
TARGET  = pong

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET) scoreboard.bin

.PHONY: all clean
