.c.o:
	gcc -c -std=gnu99 -I /usr/local/include  $<

LIBS :=  -L /usr/local/lib -lCscNet -lpthread

ALL := testHof

all: $(ALL)

testHof: hashHof.o
	gcc $< $(LIBS) -o $@

clean:
	rm $(ALL) testHof temp* *.o 2>/dev/null ; true

