CC ?= cc

CFLAGS := $(shell pkg-config --cflags gtk4 sqlite3) -Wall -Wextra -O2
LIBS := $(shell pkg-config --libs gtk4 sqlite3)

TARGET = flashcards
SRC = src/main.c src/manager.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c src/manager.h src/states.h src/util.h
	$(CC) $(CFLAGS) -c -o $@ $<

install-autostart:
	mkdir -p ~/.config/autostart
	cp flashcards.desktop ~/.config/autostart/flashcards.desktop

uninstall-autostart:
	rm -f ~/.config/autostart/flashcards.desktop

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run install-autostart uninstall-autostart
