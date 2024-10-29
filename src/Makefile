OBJECTS = main.o server.o
OUTPUT = mxim
PHONY = clean all install
CFLAGS = -Wall -O2
LIBS = -lpthread -lX11

ifeq ($(PREFIX), )
	PREFIX = /usr
endif

all: $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf $(OUTPUT) $(OBJECTS)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -oroot -groot -m755 $(OUTPUT) $(DESTDIR)$(PREFIX)/bin

uninstall:
	rm $(DESTDIR)$(PREFIX)/bin/$(OUTPUT)

.PHONY: $(PHONY)
