
.c.o:
	gcc -c -std=gnu99 -I /usr/local/include  $<

LIBS :=  -L /usr/local/lib -lCscNet -lpthread

ALL := procDict

all: $(ALL)

unicodeLkup.o: ../WebSpider/unicodeLkup.c
	gcc -c -std=gnu99 -I /usr/local/include  $^

procDict: procDict.o dict.o unicodeLkup.o
	gcc $^ $(LIBS) -o $@

dict: procDict
	./procDict /usr/share/dict/american-english /usr/share/dict/british-english dict

clean:
	rm $(ALL) *.o 2>/dev/null ; true

