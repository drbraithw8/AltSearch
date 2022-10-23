
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <CscNetLib/std.h>
#include <CscNetLib/alloc.h>
#include <CscNetLib/list.h>
#include <CscNetLib/hash.h>

#include "dict.h"
#include "../WebSpider/unicodeLkup.h"


csc_bool_t isConsonant(ch)
{	switch (ch)
	{	case 'q': case 'w': case 'r': case 't': case 'y': case 'p':
		case 's': case 'd': case 'f': case 'g': case 'h': case 'j':
		case 'k': case 'l': case 'z': case 'x': case 'c': case 'v':
		case 'b': case 'n': case 'm':
			return csc_TRUE;
		default:
			return csc_FALSE;
	}
}
		

csc_hash_t *utf8Lkup = NULL;



static int fgetline_utf8(FILE *fp, char *line, int maxLineLen)
{   register int ch, iLine;
 
/*  Look at first char for EOF. */
    ch = getc(fp);
    if (ch == EOF)
        return -1;
 
/* Read in line */
    iLine=0;
    while (ch!='\n' && ch!=EOF && iLine<maxLineLen)
	{	const char *str = unLkp_trans(utf8Lkup, ch);
		int sch = *str;
		while (iLine<maxLineLen && sch!='\0')
		{	line[iLine++] = sch;
			sch = *++str;
		}
        ch = unLkp_getUtf8(fp);
	}
    line[iLine] = '\0';
 
/* Skip any remainder of line */
    while (ch!='\n' && ch!=EOF)
	{	const char *str = unLkp_trans(utf8Lkup, ch);
		int sch = *str;
		while (sch!='\0')
		{	iLine++;
			sch = *++str;
		}
        ch = unLkp_getUtf8(fp);
	}

// Home with the bacon.
    return(iLine);
}	


// static const char *trans_win1252[256] =
// {	 "",  "",  "",  "",  "",  "",  "",  ""  /* 0 */
// ,	 "",  "","\n",  "",  "",  "",  "",  ""
// ,	 "",  "",  "",  "",  "",  "",  "",  ""  /* 1 */
// ,	 "",  "",  "",  "",  "",  "",  "",  ""
// ,	" ",  "",  "", "#",  "", "%", "&", "'"  /* 2 */
// ,	 "",  "",  "",  "",  "", "-", ".", "/"
// ,	"0", "1", "2", "3", "4", "5", "6", "7"  /* 3 */
// ,	"8", "9", ":", ";",  "", "=",  "", "?"
// ,	"@", "A", "B", "C", "D", "E", "F", "G"  /* 4 */
// ,	"H", "I", "J", "K", "L", "M", "N", "O"
// ,	"P", "Q", "R", "S", "T", "U", "V", "W"  /* 5 */
// ,	"X", "Y", "Z",  "",  "",  "",  "",  ""
// ,   "'", "a", "b", "c", "d", "e", "f", "g"  /* 6 */
// ,	"h", "i", "j", "k", "l", "m", "n", "o"
// ,	"p", "q", "r", "s", "t", "u", "v", "w"  /* 7 */
// ,	"x", "y", "z",  "",  "",  "",  "",  ""
// ,	 "",  "",  "", "f",  "",  "",  "",  ""  /* 8 */
// ,	 "",  "",  "",  "", "CE", "", "Z",  ""
// ,	 "", "'", "'",  "",  "",  "",  "",  ""  /* 9 */
// ,	 "",  "",  "",  "", "oe", "", "z", "Y"
// ,	 "",  "",  "",  "",  "",  "",  "",  ""  /* A */
// ,	 "",  "",  "",  "",  "",  "",  "",  ""
// ,	 "",  "",  "",  "",  "",  "",  "",  ""  /* B */
// ,	 "",  "",  "",  "",  "",  "",  "", "."
// ,	"A", "A", "A", "A", "A", "A", "AE","C"  /* C */
// ,	"E", "E", "E", "E", "I", "I", "I", "I"
// ,  "th", "N", "O", "O", "O", "O", "O",  ""  /* D */
// ,	"0", "U", "U", "U", "U", "Y","TH", "S"
// ,	"a", "a", "a", "a", "a", "a","ae", "c"  /* E */
// ,	"e", "e", "e", "e", "i", "i", "i", "i"
// ,  "th", "n", "o", "o", "o", "o", "o",  ""  /* F */
// ,	"0", "u", "u", "u", "u", "y", "th", ""
// };
// 
// 
// int fgetline_w1252(FILE *fp, char *line, int maxLineLen)
// {   register int ch, iLine;
//  
// /*  Look at first char for EOF. */
//     ch = getc(fp);
//     if (ch == EOF)
//         return -1;
//  
// /* Read in line */
//     iLine=0;
//     while (ch!='\n' && ch!=EOF && iLine<maxLineLen)
// 	{	char *str = (char*)trans_win1252[ch];
// 		int sch = *str;
// 		while (iLine<maxLineLen && sch!='\0')
// 		{	line[iLine++] = sch;
// 			sch = *++str;
// 		}
//         ch = getc(fp);
// 	}
//     line[iLine] = '\0';
//  
// /* Skip any remainder of line */
//     while (ch!='\n' && ch!=EOF)
// 	{	char *str = (char*)trans_win1252[ch];
// 		int sch = *str;
// 		while (sch!='\0')
// 		{	iLine++;
// 			sch = *++str;
// 		}
//         ch = getc(fp);
// 	}
// 
// // Home with the bacon.
//     return(iLine);
// }


char *dictEntry_new(char *lkup, char *target)
{	int entLen;
	int lkupLen;
	char *ent;
 
// length of an entry.
	lkupLen = strlen(lkup);
	if (target == NULL)
		entLen = 0;
	else
		entLen = strlen(target);
	entLen += lkupLen + 3;
 
// allocate entry.
	ent = csc_ck_malloc(entLen);
 
// assign.
	strcpy(ent+1, lkup);
	if (target == NULL)
	{	ent[0] = 0;
	}
	else 
	{	ent[0] = lkupLen + 2;
		strcpy(ent+ent[0], target);
	}
 
// Home with the bacon.
	return ent;
}


csc_hash_t *readDict(FILE *fin)
{	enum { lineMax=40 };
	char line[lineMax+1];
	char *words[3];
	char *ent;
	int nWds;
 
// Initialise the character translation.
	if (!utf8Lkup)
		utf8Lkup = unLkp_new();
	csc_assert(utf8Lkup);
 
// Create the empty dictionary.
	csc_hash_t *dict = csc_hash_new( 1
								   , csc_hash_StrCmpr
								   , csc_hash_str
								   , csc_hash_FreeBlk
								   );
	
// Read all lines into the dictionary.
	while (fgetline_utf8(fin,line,lineMax) != -1)
	{	int len = strlen(line);
		int ch;
 
	// Permit comment lines.
		if (line[0] == '#')
			continue;
 
	// Remove repeated consonants.
		int j = 1;
		ch = line[0];
		line[0] = tolower(ch);
		for (int i=1; i<len; i++)
		{	ch = line[i];
			ch = tolower(ch);
			if (line[i-1]!=ch || !isConsonant(ch))
			{	line[j++] = ch;
			}
		}
		line[j] = '\0';
		len = j;
 
	// Split line into words.
		nWds = csc_param(words, line, 3);
 
	// Create an entry from the words.
		if (nWds == 1)
		{	ent = dictEntry_new(words[0], NULL);
		}
		else if (nWds == 2)
		{	ent = dictEntry_new(words[0], words[1]);
		}
		else
		{	continue;
		}
 
 	// Add the entry into the dictionary.
		if (!csc_hash_addex(dict, ent))
		{	free(ent);
		}
	}
 
	return dict;
}


csc_hash_t *readDictPath(const char *path)
{	csc_hash_t *dict;
	FILE *fin = fopen(path, "r");
	if (fin == NULL)
	{	return NULL;
	}
	dict = readDict(fin);
	fclose(fin);
	return dict;
}


