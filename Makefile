DEBUG=-g
CC=gcc
CFLAGS=-Wall -Wextra -pedantic \
	   $(shell pkg-config --cflags gtk4 glib-2.0 json-glib-1.0 libadwaita-1) \
	   $(DEBUG)
LDFLAGS=$(shell pkg-config --libs gtk4 glib-2.0 json-glib-1.0 libadwaita-1) -lcurl
BINARY=main
SOURCE=./src/*.c

$(BINARY): $(SOURCE)
	$(CC) $(SOURCE) -o $(BINARY) $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(BINARY) $(OBJECTS)
