CC = gcc
CFLAGS = -Wall -Wextra -O2 $(shell pkg-config --cflags json-c libcurl)
LDLIBS = $(shell pkg-config --libs json-c libcurl)

SRCS = src/main.c src/config.c src/api.c src/trigger.c
OBJS = $(SRCS:.c=.o)

TARGET = gtwc

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

src/%.o: src/%.c src/%.h
	$(CC) $(CFLAGS) -c -o $@ $<

src/main.o: src/main.c src/config.h src/api.h src/trigger.h

clean:
	rm -f $(OBJS) $(TARGET)

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)/usr/local/bin/$(TARGET)

uninstall:
	rm -f $(DESTDIR)/usr/local/bin/$(TARGET)
