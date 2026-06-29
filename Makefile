CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -O2 $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs) -lSDL2_ttf -lm
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
