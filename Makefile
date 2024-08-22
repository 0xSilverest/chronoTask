CC = gcc
CFLAGS = -Wall -Wextra -I./include -I/usr/include/freetype2 -I/usr/include/yaml -I/usr/include/SDL2
LIBS = -lX11 -lXinerama -lXft -lfontconfig -lfreetype -lyaml -lSDL2 -lSDL2_mixer

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TARGET = chronoTask

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

clean:
	$(MAKE) -C ctrl clean
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean ctrl

all: $(TARGET) ctrl

ctrl:
	$(MAKE) -C ctrl

