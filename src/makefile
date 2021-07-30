CC=gcc
CFLAGS=-I$(IDIR)

IDIR=../include

_SDEPS=util.h filenames.h
_CDEPS=util.h
SDEPS=$(patsubst %, $(IDIR)/%, $(_SDEPS))
CDEPS=$(patsubst %, $(IDIR)/%, $(_CDEPS))

ifneq ($(_OUTLOC), ../tests/)
	override _OUTLOC=./
endif

OBJ=$(patsubst %,$(ODIR)/%)
ODIR=obj
UTILOBJ=obj/util.o
FNOBJ=obj/filenames.o

$(ODIR)/%.o: %.c $(SDEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server: $(ODIR)/server.o $(UTILOBJ) $(FNOBJ)
	$(CC) -o $(_OUTLOC)$@ $^ $(CFLAGS)

client: $(ODIR)/client.o $(UTILOBJ)
	$(CC) -o $(_OUTLOC)$@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm $(ODIR)/*.o ./*.txt client server 