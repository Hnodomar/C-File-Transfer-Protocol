LINKCHECK = `pkg-config --cflags --libs check`
UNIT_TEST = unit_tests
DEPSOBJ = util.o filenames.o
INC = -I../../include
CC = gcc

$(UNIT_TEST): $(UNIT_TEST).o $(DEPSOBJ)
	$(CC) -o $@ $^ $(LINKCHECK) $(INC)
util.o: ../../src/util.c
	$(CC) -c -o $@ $< $(INC)
filenames.o: ../../src/filenames.c
	$(CC) -c -o $@ $< $(INC)
%.o: %.c
	$(CC) -c -o $@ $<

.PHONY: clean

clean:
	rm *.o