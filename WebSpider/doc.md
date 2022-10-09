WebSpider
=========
The code in here is intended to download and digest websites.
design.txt
------------

makefile
------------

inch.c inch.h
------------
*	It does the input.  Deals with charset.
~~~
	int inch_curCh(inch_t *inch);
	int inch_next(inch_t *inch);
~~~
*	Has a limited look-ahead
~~~
	char *inch_lookAhead(inch_t *inch, char buf[5]);
~~~

procHtml.c
------------
*	HTML parsing, but passes body plaintext to bodychar.c

bodychar.c
------------
*	Interface accepts a single character.
~~~
	void doCh(bodyCh_t *bc, int ch)
~~~
*	Deals with charset a bit more (in addition to that in inch.c).
*	Deals with escapes and URLs.

server.c.
------------
It is to be a server to pull and process websites.  It uses:-
*	csc_ini
*	csc_log

