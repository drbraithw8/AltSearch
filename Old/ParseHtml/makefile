.c.o:
	gcc -c -std=gnu99 -I /usr/local/include  $<

LIBS :=  -L /usr/local/lib -lCscNet -lpthread

ALL := htmlStrip

all: $(ALL)

htmlStrip: strip.o
	gcc $^ $(LIBS) -o $@  

clean:
	rm *.o 2>/dev/null ; true

cleanall: 
	rm $(ALL) *.o 2>/dev/null ; true
