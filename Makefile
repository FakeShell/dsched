CC = gcc
CFLAGS = -Wall -Wextra -pedantic
TARGET = dsched
OBJ = dsched.o
SRCDIR = src

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $<

dsched.o: $(SRCDIR)/dsched.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all clean
