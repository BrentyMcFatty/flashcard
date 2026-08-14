# GTK Flashcards

A small GTK 4 + SQLite desktop flashcard program designed to run quietly
after the user logs into their desktop.

## Behavior

- There is NO in-app login system.
- The operating system's desktop login starts the program through the
  autostart `.desktop` file.
- The program waits 30 seconds after startup.
- It then shows one random flashcard.
- It does not continuously interrupt the user with cards.
- "Reveal answer" shows the answer.
- "Next" selects another random card.
- "Later" minimizes the window.
- "Manage cards" opens a small editor for adding cards.
- Cards are stored in `flashcards.db`.

## Dependencies

Debian/Ubuntu:

    sudo apt install build-essential pkg-config libgtk-4-dev libsqlite3-dev

Fedora:

    sudo dnf install gcc make pkg-config gtk4-devel sqlite-devel

Arch:

    sudo pacman -S base-devel pkgconf gtk4 sqlite

## Build

    make

Run manually:

    ./flashcards

## Enable automatic startup on Linux

First find the full path:

    pwd

Then edit `flashcards.desktop` and replace:

    REPLACE_WITH_FULL_PATH

with the directory containing the compiled `flashcards` executable.

For example:

    Exec=/home/alex/gtk_flashcards/flashcards

Then:

    make install-autostart

The program will start automatically when the desktop session begins.

To disable automatic startup:

    make uninstall-autostart

## Important note about "not disturbing" the user

The application intentionally does NOT use "always on top" and does not
force itself over fullscreen applications.

Desktop/window-manager behavior can vary, but this design leaves normal
focus behavior to the desktop environment. If you want even gentler
behavior, the next version could use a desktop notification first and only
open the card when clicked.

## Changing the delay

At the top of `main.c`:

    #define DEFAULT_DELAY_SECONDS 30

Change `30` to another number of seconds, for example:

    #define DEFAULT_DELAY_SECONDS 60

for a one-minute delay.
