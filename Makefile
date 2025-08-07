DEBUG=-g
CC=gcc
CFLAGS=-Wall -Wextra -pedantic \
	-O3 -march=native -ffast-math \
	$(shell pkg-config --cflags gtk4 glib-2.0 json-glib-1.0 libadwaita-1) \
	$(DEBUG)
LDFLAGS=$(shell pkg-config --libs gtk4 glib-2.0 json-glib-1.0 libadwaita-1) -lcurl
BINARY=nekodownloader
SOURCE=./src/*.c

$(BINARY): clean gresources
	$(CC) $(SOURCE) -o $(BINARY) $(CFLAGS) $(LDFLAGS)

gresources:
	glib-compile-resources src/nekodownloader.gresource.xml \
		--sourcedir=ui \
		--generate-source

clean:
	rm -f $(BINARY)
	rm -f ./src/nekodownloader.c
