
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <CscNetLib/std.h>
#include <CscNetLib/alloc.h>
#include "inch.h"

// Type for getting an ASCII character.
// *	Provides n character lookahead, with n in the range 1-4.
#define N_Lookahead 4

typedef struct inch_t
{	FILE *fin;
	int (*getCh)(FILE *fin);
	int que[N_Lookahead+1];
	int queFirst, queFree;
	int chPos, lineNo, linePos;
} inch_t;

#define que_isempty(inch) (inch->queFirst == inch->queFree)
#define que_isfull(inch) ((inch->queFree+1) % (N_Lookahead+1) == inch->queFirst)



static int inCh_ascii(FILE *fin)
{	return getc(fin);
}


// Constructor.  See inch_setGetAsc() for details of 2nd and 3rd arg.
inch_t *inch_new(FILE *fin)
{	inch_t *inch = csc_allocOne(inch_t);
	inch->getCh = inCh_ascii;
	inch->fin = fin;
	inch->queFirst = 0;
	inch->queFree = 0;
	inch->chPos = 1;
	inch->lineNo = 1;
	inch->linePos = 0;
	
// Pre-get the first N_Lookahead chars.
	int ch;
	for (int i=0; i<N_Lookahead; i++)
	{	ch = inch->getCh(inch->fin);
		if (ch == -1)
			break;
		inch->que[inch->queFree++] = ch;
	}
 
// If the first char is a newline, then we are at line 2.
	if (inch_curCh(inch) == '\n')
	{	inch->lineNo++;
	}
 
// Return the object.
	return inch;
}


// Destructor.
void inch_free(inch_t *inch)
{	free(inch);
}


// Get one character.  EOF indicated by -1.
int inch_curCh(inch_t *inch)
{	if (que_isempty(inch))
		return -1;
	else
		return inch->que[inch->queFirst];
}


int inch_next(inch_t *inch)
{	
// If the queue is empty, then we are finished.
	if (que_isempty(inch))
		return -1;
 
// If we are not full then it is because we met EOF.
	csc_bool_t wasFull = que_isfull(inch);
 
// Discard the character at the head of the queue.
	inch->queFirst = (inch->queFirst+1) % (N_Lookahead+1);
 
// Only get another character if we have not reached EOF.
	int ch;
	if (wasFull)  // Its no longer full cos we discarded one character.
		ch = inch->getCh(inch->fin);
	else
		ch = -1;
 
// Only insert the next character if we have not reached EOF.
	if (ch == -1)
	{	if (que_isempty(inch))
			return -1;
	}
	else
	{	inch->que[inch->queFree] = ch;
		inch->queFree = (inch->queFree+1) % (N_Lookahead+1);
	}
 
// Count chars and lines.
	inch->chPos++;
	ch = inch->que[inch->queFirst];
	if (ch == '\n')
	{	inch->lineNo++;
		inch->linePos = 0;
	}
	else
	{	inch->linePos++;
	}
	
// Home with the goods.
	return ch;
}


void inch_reportPos(inch_t *inch, FILE *fout)
{	fprintf( fout, "inchPos: c=%d l=%d p=%d ch=%d\n"
		   , inch->chPos, inch->lineNo, inch->linePos, inch_curCh(inch));
}


// Lookahead n chars.  Writes null terminated string into buf, which must
// have size 5.  Null terminated string may be short as we approach the EOF.
char *inch_lookAhead(inch_t *inch, char buf[5])
{	int iBuf = 0;
	int iFirst = inch->queFirst;
	int iFree = inch->queFree;
	while (iFirst != iFree)
	{	buf[iBuf++] = inch->que[iFirst];
		iFirst = (iFirst+1) % (N_Lookahead+1);
	}
	buf[iBuf] = '\0';
	return buf;
}


// Function for getting the character, i.e. translating to ASCII from:-
// 	*	windows-1252
// 	*	UTF8
// 	*	ASCII (no-op)
csc_bool_t inch_set_getAscChar(inch_t *inch, const char *charset)
{	csc_bool_t isOk = csc_TRUE;
 
    if (  csc_strieq(charset,"windows-1252")
       || csc_strieq(charset,"ansi")
       || csc_strieq(charset,"iso-8859-1")
       || csc_strieq(charset,"ascii")
       )
	{	inch->getCh = inCh_ascii;
	}
	else if (csc_strieq(charset,"UTF-8"))
	{	inch->getCh = unLkp_getUtf8;
	}
	else
	{	isOk = csc_FALSE;
	}

	return isOk;
}


// // *	getCh() is a function for getting the character, i.e. translating to
// // 	ASCII from:-
// // 	*	windows-1252
// // 	*	UTF8
// // 	*	ASCII (no-op)
// void inch_set_getAscChar( inch_t *inch
// 						, int (*getCh)(void *context)
// 						, void *getAscChar_context
// 						)
// {	inch->getCh = getCh;
// 	inch->getAscChar_context = getAscChar_context;
// }


// typedef struct test_getAscChar_t 
// {	char *testStr;
// 	int pos;
// } test_getAscChar_t;
// test_getAscChar_t test_getAscCh;
// int test_getAscChar(test_getAscChar_t *context)
// {	int ch = context->testStr[context->pos];
// 	if (ch == '\0')
// 	{	return -1;
// 	}
// 	else
// 	{	context->pos++;
// 		return ch;
// 	}
// }
// void main(int argc, char **argv)
// {	csc_assert(argc == 2);
// 	test_getAscCh.testStr = argv[1];	
// 	test_getAscCh.pos = 0;
// 	inch_t *inch = inch_new( (int(*)(void *context)) test_getAscChar
// 						   , (void*)&test_getAscCh
// 						   );
// 	char buf[5];
// 	int ch = inch_curCh(inch);
// 	while (ch != -1)
// 	{	printf("%c \"%s\"\n", ch, inch_lookAhead(inch, buf));
// 		ch = inch_next(inch);
// 	}
// 	printf("@ \"%s\"\n", inch_lookAhead(inch, buf));
// }


// -------------------------------------------------

