OBJECTS = main.o xhandler.o thread.o ximserver.o fd.o in4.o ximclient.o \
	  inputmethod.o inputcontext.o ximtypes.o ximproto.o keysym.o   \
	  config.o segment.o
OUTPUT = mxim
PHONY = clean all install
CFLAGS = -Wall -g
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
