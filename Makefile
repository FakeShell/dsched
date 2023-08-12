CC = gcc
CFLAGS = $(shell pkg-config --cflags glib-2.0 gio-2.0)
LIBS = $(shell pkg-config --libs glib-2.0 gio-2.0)
TARGET = dsched
SRCDIR = src
SRCFILES = $(SRCDIR)/dsched.c $(SRCDIR)/dbus_listener.c $(SRCDIR)/generated-code.c

all: $(TARGET)

$(TARGET): $(SRCFILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
