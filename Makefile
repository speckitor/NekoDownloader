DEBUG ?= -g
CC = gcc
CFLAGS = -Wall -Wextra -pedantic \
	-O3 -march=native -ffast-math \
	$(shell pkg-config --cflags gtk4 glib-2.0 json-glib-1.0 libadwaita-1) \
	$(DEBUG)
LDFLAGS = $(shell pkg-config --libs gtk4 glib-2.0 json-glib-1.0 libadwaita-1) -lcurl -lwebp

BINARY=nekodownloader
SOURCE=./src/*.c

PREFIX ?= /usr/local
BIN_DIR = $(PREFIX)/bin
DESKTOP_DIR = $(PREFIX)/share/applications
ICON_DIR = $(PREFIX)/share/icons/hicolor/scalable/apps
METAINFO_DIR = $(PREFIX)/share/metainfo

$(BINARY): gresources
	$(CC) $(SOURCE) -o $(BINARY) $(CFLAGS) $(LDFLAGS)

gresources:
	glib-compile-resources src/nekodownloader.gresource.xml \
		--sourcedir=data/ui \
		--generate-source

install: $(BINARY)
	install -Dm755 $(BINARY) $(BIN_DIR)
	install -Dm644 data/org.speckitor.NekoDownloader.desktop $(DESKTOP_DIR)/org.speckitor.NekoDownloader.desktop
	install -Dm644 data/org.speckitor.NekoDownloader.svg $(ICON_DIR)/org.speckitor.NekoDownloader.svg

uninstall:
	rm -f $(BIN_DIR)/$(BINARY)
	rm -f $(DESKTOP_DIR)/org.speckitor.NekoDownloader.desktop
	rm -f $(ICON_DIR)/org.speckitor.NekoDownloader.svg

clean:
	rm -f $(BINARY)
	rm -r ./src/nekodownloader.c
