CC=gcc
CFLAGS=-I$(IDIR)
_DEPS=util.h
DEPS=$(patsubst %,$(IDIR)/%,$(_DEPS))
OBJ=$(patsubst %,$(ODIR)/%)

ODIR=obj
UTILOBJ=obj/util.o
IDIR=../include

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server: $(ODIR)/server.o $(UTILOBJ)
	$(CC) -o $@ $^ $(CFLAGS)

client: $(ODIR)/client.o $(UTILOBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm $(ODIR)/*.o client server 