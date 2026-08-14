CC ?= cc

CFLAGS := $(shell pkg-config --cflags gtk4 sqlite3) -Wall -Wextra -O2
LIBS := $(shell pkg-config --libs gtk4 sqlite3)

TARGET = flashcards
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

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
