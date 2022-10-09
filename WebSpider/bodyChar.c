
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <CscNetLib/std.h>
#include <CscNetLib/alloc.h>
#include <CscNetLib/hash.h>
#include <CscNetLib/isvalid.h>
#include "inch.h"
#include "bodyChar.h"
#include "../Plurals/dict.h"



// -------- Windows 1252 -----------------------------------

static const char *trans_win1252[256] =
{	" ", " ", " ", " ", " ", " ", " ", " "  /* 0 */
,	" ", " ", " ", " ", " ", " ", " ", " "
,	" ", " ", " ", " ", " ", " ", " ", " "  /* 1 */
,	" ", " ", " ", " ", " ", " ", " ", " "
,	" ", " ", " ", "#", " ", "%", "&", "'"  /* 2 */
,	" ", " ", " ", " ", " ", "-", ".", "/"
,	"0", "1", "2", "3", "4", "5", "6", "7"  /* 3 */
,	"8", "9", ":", ";", " ", "=", " ", "?"
,	"@", "A", "B", "C", "D", "E", "F", "G"  /* 4 */
,	"H", "I", "J", "K", "L", "M", "N", "O"
,	"P", "Q", "R", "S", "T", "U", "V", "W"  /* 5 */
,	"X", "Y", "Z", " ", " ", " ", " ", " "
,   "'", "a", "b", "c", "d", "e", "f", "g"  /* 6 */
,	"h", "i", "j", "k", "l", "m", "n", "o"
,	"p", "q", "r", "s", "t", "u", "v", "w"  /* 7 */
,	"x", "y", "z", " ", " ", " ", " ", " "
,	" ", " ", " ", "f", " ", " ", " ", " "  /* 8 */
,	" ", " ", " ", " ", "CE"," ", "Z", " "
,	" ", "'", "'", " ", " ", " ", " ", " "  /* 9 */
,	" ", " ", " ", " ", "oe"," ", "z", "Y"
,	" ", " ", " ", " ", " ", " ", " ", " "  /* A */
,	" ", " ", " ", " ", " ", " ", " ", " "
,	" ", " ", " ", " ", " ", " ", " ", " "  /* B */
,	" ", " ", " ", " ","\"", " ", " ", "."
,	"A", "A", "A", "A", "A", "A", "AE","C"  /* C */
,	"E", "E", "E", "E", "I", "I", "I", "I"
,	"th", "N", "O", "O", "O", "O", "O", " "  /* D */
,	"0", "U", "U", "U", "U", "Y", "TH", "S"
,	"a", "a", "a", "a", "a", "a", "ae","c"  /* E */
,	"e", "e", "e", "e", "i", "i", "i", "i"
,	"th", "n", "o", "o", "o", "o", "o", " "  /* F */
,	"0", "u", "u", "u", "u", "y", "th", ""
};

 
static const char *trans_windows1252(csc_hash_t *context, int ch)
{	if ((unsigned)ch < 256)
		return trans_win1252[ch];
	else
		return " ";
}

// -------- UTF8 -----------------------------------

typedef struct unLkpEnt_t
{	uint32_t uVal;  // Unicode Value. 
	char asc[4];   // ASCII equivalent string.
} unLkpEnt_t;  // Unicode Lookup Entry


static int unLkp_cmp(void *a, void *b)
{	return (*(uint32_t*)a != *(uint32_t*)b);
}


static uint64_t unLkp_hval(void *a)
{	return *(uint32_t*)a;
}
	

// Reads in the utf8 translation table.
// Returns NULL on error.
static csc_hash_t *unLkp_new()
{	enum {	LineMax = 10
		 ,	MaxWords = 3
		 };
	char line[LineMax+1];
	char *words[MaxWords];
 
// Open source file.
	FILE *fin = fopen("unicodeLkup.txt", "r");
	if (fin == NULL)
		return NULL;
 
// New hash table.
	csc_hash_t *ul = csc_hash_new(0, unLkp_cmp, unLkp_hval, csc_hash_FreeBlk);
 
// Read in entries.
	while (csc_fgetline(fin,line,LineMax) != -1)
	{
	// Ignore comment lines.
		if (line[0] == '#')
			continue;
 
	// Split line into words.
		int nwds = csc_param(words, line, MaxWords);
 
	// Ignore blank lines.
		if (nwds == 0)
		{	continue;
		}
 
	// Correct number of words?
		if (nwds != 2)
		{	csc_hash_free(ul);
			ul = NULL;
			break;
		}
 
	// Look at hex value.
		char *hexVal = words[0];
		if (strlen(hexVal)!=4 || !csc_isValid_hex(hexVal))
		{	csc_hash_free(ul);
			ul = NULL;
			break;
		}
 
	// Look at the ascii string.
		if (strlen(words[1]) > 2)
		{	csc_hash_free(ul);
			ul = NULL;
			break;
		}
 
	// Create new entry, and add it to the table.
		unLkpEnt_t *ent = csc_allocOne(unLkpEnt_t);
		ent->uVal = strtol(words[0],NULL,16);
		strcpy(ent->asc, words[1]);
		if (!csc_hash_addex(ul, ent))
		{	fprintf(stderr, "unLkpt DUP: %x\n", ent->uVal);
			free(ent);
		}
	}
 
// Clean up.
	fclose(fin);
	return ul;
}


static const char *unLkp_trans(csc_hash_t *ul, int ch)
{	unLkpEnt_t *ent = csc_hash_get(ul, &ch);
	if (ent == NULL)
	{	return " ";
	}
	else
	{	return ent->asc;
	}
}


// void testUl(csc_hash_t *ul, int ch, const char *expected)
// {	const char *str = unLkp_get(ul,ch);
// 	if (strcmp(str, expected) == 0)
// 	{	fprintf(stdout, "%x_%s pass\n", ch, expected);
// 	}
// 	else
// 	{	fprintf(stdout, "%x_%s FAIL\n", ch, expected);
// 	}
// }
// void main(int argc, char **argv)
// {	csc_hash_t *ul = unLkp_new();
// 	csc_assert(ul);
// 	testUl(ul, 0, " ");
// 	testUl(ul, 1, " ");
// 	testUl(ul, 32, " ");
// 	testUl(ul, 37, "%");
// 	testUl(ul, 38, "&");
// 	testUl(ul, 39, "\'");
// 	testUl(ul, 45, "-");
// 	testUl(ul, 46, ".");
// 	testUl(ul, 48, "0");
// 	testUl(ul, 57, "9");
// 	testUl(ul, 58, ":");
// 	testUl(ul, 65, "A");
// 	testUl(ul, 90, "Z");
// 	testUl(ul, 91, " ");
// 	testUl(ul, 96, "\'");
// 	testUl(ul, 97, "a");
// 	testUl(ul, 122, "z");
// 	testUl(ul, 0xC0, "A");
// 	testUl(ul, 0xD0, "D");
// 	testUl(ul, 0xEA, "e");
// 	testUl(ul, 0xFA, "u");
// 	testUl(ul, 0x1C4, "DZ");
// 	testUl(ul, 0x2A6, "ts");
// 	testUl(ul, 0x2B9, "'");
// 	testUl(ul, 0x2BB, "'");
// 	testUl(ul, 0x2BC, "'");
// 	testUl(ul, 0x2C8, "'");
// 	testUl(ul, 0x2CA, "'");
// 	testUl(ul, 0x2CB, "'");
// 	testUl(ul, 0x2F4, "'");
// 	testUl(ul, 0x300, "'");
// 	testUl(ul, 0x301, "'");
// 	testUl(ul, 0x30D, "'");
// 	testUl(ul, 0x312, "'");
// 	testUl(ul, 0x313, "'");
// 	testUl(ul, 0x314, "'");
// 	testUl(ul, 0x340, "'");
// 	testUl(ul, 0x341, "'");
// 	testUl(ul, 0x2019, "'");
// 	testUl(ul, 0x2018, "'");
// 	testUl(ul, 0x201B, "'");
// 	testUl(ul, 0x2032, "'");
// 	testUl(ul, 0x2035, "'");
// 	csc_hash_free(ul);
// 	csc_mck_print(stderr);
// 	exit(0);
// }



// ============ Body Char ===============================

enum { bodyCh_transStrMax=15
	 , bodyCh_wordMax=32
	 , bodyCh_ampMax=12
	 };
typedef enum { bodyCh_state_None=0
			 , bodyCh_state_Url=1
			 , bodyCh_state_Amp=2
			 } bodyCh_state_t;
typedef struct bodyCh_t
{	char transStr[bodyCh_transStrMax+1];
	const char *(*translate)(csc_hash_t *unLkpTbl, int ch);
	csc_hash_t *unLkpTbl;
	char word[bodyCh_wordMax+1];
	char amp[bodyCh_ampMax+1];
	int iWord, iWdCount, iAmp;
	csc_bool_t isNewSentance;
	csc_bool_t isCap;
	bodyCh_state_t state;
	csc_hash_t *words;
	csc_hash_t *trivial;
	csc_hash_t *dict;
} bodyCh_t;


bodyCh_t *bodyCh_new(const char *transStr)
{
// Basic.
	bodyCh_t *bc = csc_allocOne(bodyCh_t);
	bc->iWord = 0;
	bc->iWdCount = 0;
	bc->state = bodyCh_state_None;
	bc->isNewSentance = csc_TRUE;
	bc->isCap = csc_TRUE;
 
// Dictionaries.
	bc->dict = readDictWords();
    bc->words = csc_hash_new( 0
                            , csc_hash_StrCmpr
                            , csc_hash_str
                            , free
                            );
	// bc->trivial = readTrivialWords();
 
// Translation function.
	bc->unLkpTbl = unLkp_new();
	bodyCh_setTrans(bc, transStr);
	return bc;
}


void bodyCh_setTrans(bodyCh_t *bc, const char *transStr)
{	char *word = bc->word;
	int len = strlen(word);

	if (csc_strieq(transStr,"utf8"))
	{
		csc_strncpy(bc->transStr, transStr, bodyCh_transStrMax);
		bc->translate = trans_windows1252;
	}
	else
	{
		csc_strncpy(bc->transStr, transStr, bodyCh_transStrMax);
		bc->translate = trans_windows1252;
	}
}


void bodyCh_free(bodyCh_t *bc)
{
// 	csc_hash_free(bc->trivial);
	csc_hash_free(bc->words);
	csc_hash_free(bc->dict);
	csc_hash_free(bc->unLkpTbl);
	free(bc);
}


static void bodyCh_word(bodyCh_t *bc)
{	char *word = bc->word;
	int len = strlen(word);
	
// Convert word to lowercase.
	bc->isCap = isupper(word[0]);
	for (int i=0; i<len; i++)
	{	word[i] = tolower(word[i]);
	}
 
// Look up the word in the dictionary.
	char *wDict = csc_hash_get(dict, word);
	if (wDict != NULL)
	{	if (wDict[0] > 0)
		{	word = wDict + wDict[0];
		}
	}
	else
	{	if (len>2 && csc_streq(word+len-2,"'s"))
		{	word[len-2] = '\0';  // Remove possessives.
		}
	}
 
// Add the word to the list of words.
	char *wordRec = csc_alloc_str(word);
	if (!csc_hash_addex(bc->words, wordRec))
	{	free(wordRec);
	}
}


// A piece of code from hell.  Ignores URLs.  Ignores & escapes generally,
// but interprets &# escapes.  Divides text into words.
static void doCh(bodyCh_t *bc, int ch)
{
// The state indicates if we are processing:-
// * An &..; escape.
// * A URL.
// * None of the above.
	if (bc->state == bodyCh_state_Amp)
	{	if (ch == ';')                      
		{ // The end of the escape.
			bc->state = bodyCh_state_None;   // Next paragraph deals with escaped character.
 
		// Now interpret the escaped character.
			char *str = bc->amp;
			str[bc->iAmp] = '\0';
			ch = *str++;
			if (ch=='#' && csc_isValid_int(str))
			{	str = (char*)unLkp_trans(bc->unLkpTbl, atoi(str));
				if (  strlen(str)==2
				   && bc->iWord>=0 && bc->iWord<bodyCh_wordMax
				   )
				{	bc->word[bc->iWord++] = str[0];
					ch = str[1];
				}
				else
				{	ch = str[0];
					if (ch == '&')
					{	ch = ' ';
					}
				}
			}
			else
			{	ch = ' ';
			}
		}
		else
		{ // We are still reading the characters of the escape.
			if (bc->iAmp < bodyCh_ampMax)
			{	bc->amp[bc->iAmp++] = ch;
			}
		}
	}
 
	if (bc->state == bodyCh_state_None)
	{ // Normal chars or escaped resulting char.
 
		if (isalnum(ch) || ch=='\'')
		{	if (bc->iWord < bodyCh_wordMax)
			{	bc->word[bc->iWord++] = ch;
			}	
		}
		else if (  ch == ':'
				&& (  csc_strieq(bc->word,"http")
				   || csc_strieq(bc->word,"https")
				   )
				)
		{	bc->state = bodyCh_state_Url;
			bc->iWord = 0;
		}
		else if (ch == '&')
		{	bc->state = bodyCh_state_Amp;
			bc->iAmp = 0;
		}
		else if (bc->iWord > 0)
		{	bc->word[bc->iWord] = '\0';
			bodyCh_word(bc);
			bc->iWord = 0;
			bc->isNewSentance = (ch == '.');
		}
	}
	else if (bc->state == bodyCh_state_Url)
	{	if (!isalnum(ch) && strchr("./%&?=-",ch)==NULL)
		{	bc->state = bodyCh_state_None;
		}
	}
}


void bodyCh_ch(bodyCh_t *bc, int ch)
{	if ((unsigned)ch < 128)
	{	const char *str = trans_win1252[ch];
		doCh(bc,*str);
	}
	else
	{	const char *str = bc->translate(bc->unLkpTbl,ch);
		int c = *str++;
		while (c != '\0')
		{	doCh(bc,c);
			c = *str++;
		}
	}
}
		
