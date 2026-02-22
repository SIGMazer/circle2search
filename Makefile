CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude `pkg-config --cflags gtk+-3.0 tesseract lept x11`
LDFLAGS = -L/usr/lib64 `pkg-config --libs gtk+-3.0 tesseract lept x11`

SRCDIR = src
INCDIR = include
BUILDDIR = build

TARGET = $(BUILDDIR)/circle2search
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BUILDDIR)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR) screenshot.png /tmp/circle2search_*
	pkill -f 'python3 -m http.server 8899' 2>/dev/null || true
	pkill -f 'ssh.*serveo.net' 2>/dev/null || true

run: $(TARGET)
	$(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

uninstall:
	rm -f /usr/local/bin/circle2search

.PHONY: all clean run install uninstall
