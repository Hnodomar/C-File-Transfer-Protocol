CC=gcc
CFLAGS=-I$(IDIR)
IDIR=../include
_SDEPS=util.h filenames.h
_CDEPS=util.h
SDEPS=$(patsubst %, $(IDIR)/%, $(_SDEPS))
CDEPS=$(patsubst %, $(IDIR)/%, $(_CDEPS))
LDFLAGS=-pthread
ODIR=obj
UTILOBJ=obj/util.o
FNOBJ=obj/filenames.o

ifneq ($(_OUTLOC), ../tests/integration/)
	override _OUTLOC=./
endif

$(ODIR)/%.o: %.c $(SDEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server: $(ODIR)/server.o $(UTILOBJ) $(FNOBJ)
	$(CC) -o $(_OUTLOC)$@ $(LDFLAGS) $^ $(CFLAGS)

client: $(ODIR)/client.o $(UTILOBJ)
	$(CC) -o $(_OUTLOC)$@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm $(ODIR)/*.o ./*.txt client server 
