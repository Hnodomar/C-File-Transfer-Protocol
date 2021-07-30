CC=gcc
CFLAGS=-I$(IDIR)

IDIR=../include

_SDEPS=util.h filenames.h
_CDEPS=util.h
SDEPS=$(patsubst %, $(IDIR)/%, $(_SDEPS))
CDEPS=$(patsubst %, $(IDIR)/%, $(_CDEPS))

OBJ=$(patsubst %,$(ODIR)/%)

ODIR=obj
UTILOBJ=obj/util.o
FNOBJ=obj/filenames.o

$(ODIR)/%.o: %.c $(SDEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server: $(ODIR)/server.o $(UTILOBJ) $(FNOBJ)
	$(CC) -o $@ $^ $(CFLAGS)

client: $(ODIR)/client.o $(UTILOBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm $(ODIR)/*.o ./*.txt client server 