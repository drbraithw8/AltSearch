
.c.o:
	gcc -c -std=gnu99 -I /usr/local/include  $<

LIBS :=  -L /usr/local/lib -lCscNet -lpthread

ALL := procHtml

all: $(ALL)

procHtml: server.o bodyChar.o inch.o procHtml.o unicodeLkup.o
	gcc $^ $(LIBS) -o $@

clean:
	rm $(ALL) *.o 2>/dev/null ; true

