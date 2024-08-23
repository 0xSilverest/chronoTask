CC = gcc
CFLAGS = -Wall -Wextra -I./include -I/usr/include/freetype2 -I/usr/include/yaml -I/usr/include/SDL2
LIBS = -lX11 -lXinerama -lXft -lfontconfig -lfreetype -lyaml -lSDL2 -lSDL2_mixer

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TARGET = chronotask

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
SYSCONFDIR = $(PREFIX)/etc/chronotask

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

clean:
	$(MAKE) -C ctrl clean
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean ctrl install

all: $(TARGET) ctrl

ctrl:
	$(MAKE) -C ctrl

install: $(TARGET) ctrl
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	install -m 755 ctrl/chronotask-ctrl $(DESTDIR)$(BINDIR)/chronotask-ctrl
	install -d $(DESTDIR)$(SYSCONFDIR)
	install -m 644 config.yaml $(DESTDIR)$(SYSCONFDIR)/config.yaml.example
	install -m 644 routines.yaml $(DESTDIR)$(SYSCONFDIR)/routines.yaml.example
	@echo "ChronoTask has been installed to $(DESTDIR)$(BINDIR)"
	@echo "Example configuration files have been installed to $(DESTDIR)$(SYSCONFDIR)"
	@echo "To set up your personal configuration, run:"
	@echo "  mkdir -p ~/.config/chronotask"
	@echo "  cp $(DESTDIR)$(SYSCONFDIR)/config.yaml.example ~/.config/chronotask/config.yaml"
	@echo "  cp $(DESTDIR)$(SYSCONFDIR)/routines.yaml.example ~/.config/chronotask/routines.yaml"
	@echo "Then edit these files to customize your setup."
