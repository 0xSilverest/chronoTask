CC = gcc
CFLAGS = -Wall -Wextra -I../include
SRCS = chronotask-ctrl.c ../src/socket.c
OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(notdir $(SRCS)))
TARGET = chronotask-ctrl
OBJ_DIR = obj

$(shell mkdir -p $(OBJ_DIR))

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -DCHRONOTASK_CTRL

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ -DCHRONOTASK_CTRL

$(OBJ_DIR)/%.o: ../src/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -DCHRONOTASK_CTRL

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: clean

