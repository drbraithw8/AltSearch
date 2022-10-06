
.c.o:
	gcc -c -std=gnu99 -I /usr/local/include  $<

LIBS :=  -L /usr/local/lib -lCscNet -lpthread

ALL := procDict

all: $(ALL)

procDict: procDict.o dict.o
	gcc $^ $(LIBS) -o $@

clean:
	rm $(ALL) *.o 2>/dev/null ; true

