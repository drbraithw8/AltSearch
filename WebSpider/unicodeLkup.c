#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <CscNetLib/std.h>
#include <CscNetLib/alloc.h>
#include <CscNetLib/hash.h>
#include <CscNetLib/isvalid.h>


// Gets a UTF8 character.
// Ignores (tries to skips over) illegal characters.
int unLkp_getUtf8(FILE *fin)
{	csc_bool_t isOk = csc_FALSE;
	int ch;
	enum
	{	Mask = 31
	,	Cmpr = 6
	,	Shft = 5
	,	ShftMin = 3
	,	ShftN = 6
	,	SixBits = 63
	};
 
	while (!isOk)
	{	int byteCount;
		int byte = getc(fin);
 
		if (byte < 128)
		{	return byte;
		}
		else
		{	int iShft;
			int mask = Mask;
			int cmpr = Cmpr;
			for (iShft=Shft; iShft>=ShftMin; iShft--)
			{	if (byte>>iShft == cmpr)
				{	ch = byte & mask;
					break;
				}
				cmpr = (cmpr+1)<<1;
				mask >>= 1;
			}
			if (iShft < ShftMin)
			{	isOk = csc_FALSE;
				byteCount = 0;
			}
			else
			{	isOk = csc_TRUE;
				byteCount = (Shft+1) - iShft;
			}
		}
 
		int i;
		for (i=0; i<byteCount; i++)
		{	byte = getc(fin);
			if (byte == -1)
			{	return -1;
			}
			else if (byte>>ShftN == 2)
			{	ch = (ch<<ShftN) + (byte & SixBits);
			}
			else
			{	break;
			}
		}
		if (i != byteCount)
		{	isOk = csc_FALSE;
		}
	}
 
	return ch;
}
// static int utf8_putc(int ch, FILE *fout)
// {	if(ch < 0x80) // Single byte-width
//     {	fputc(ch, fout);
//         return 1;
//     }
//     else if(ch < 0x800) // Double byte-width
//     {	fputc((ch>>6) | 0300, fout);
//         fputc((ch & 077) | 0200, fout);
//         return 2;
//     }
//     else if(ch < 0x10000) // Triple byte-width
//     {	fputc((ch>>12) | 0340, fout);
//         fputc(((ch>>6) & 077) | 0200, fout);
//         fputc((ch & 077) | 0200, fout);
//         return 3;
//     }
//     else if(ch < 0x200000) // Quadruple byte-width
//     {	fputc((ch>>18) | 0360, fout);
//         fputc(((ch>>12) & 077) | 0200, fout);
//         fputc(((ch>>6) & 077) | 0200, fout);
//         fputc((ch & 077) | 0200, fout);
//         return 4;
//     }
//     return 0;
// }
// void main(int argc, char **argv)
// {	char *testSeq[] = {"", "a", "abaZaa", "Ώ", "ΏΏΏΏ", "ﬆ", "ﬆﬆﬆ", "ΏﬆaaﬆΏa"};
// 	FILE *fin, *fout;
// 	int ch;
//  
// 	int nStr = csc_dim(testSeq);
// 	for (int i=0; i<nStr; i++)
// 	{ 
// 	// Write the file.
// 		fout = fopen("temp1.txt", "w");
// 		fprintf(fout, "%s", testSeq[i]);
// 		fclose(fout);
//  
// 	// Read it in and out char by char.
// 		fin  = fopen("temp1.txt", "r");
// 		fout = fopen("temp2.txt", "w");
// 		ch = inCh_utf8(fin);
// 		while (ch != -1)
// 		{	utf8_putc(ch, fout);
// 			ch = inCh_utf8(fin);
// 		}
// 		fclose(fin);
// 		fclose(fout);
//  
// 	// Read it back in as a string.
// 		fin  = fopen("temp2.txt", "r");
// 		char str[20+1];
// 		fgets(str, 20, fin);
// 		if (strcmp(str,testSeq[i]) == 0)
// 		{	fprintf(stdout, "utf8_%d pass\n", i);
// 		}
// 		else
// 		{	fprintf(stdout, "utf8_%d FAIL\n", i);
// 		}
// 		fclose(fin);
// 	}
// }


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
csc_hash_t *unLkp_new()
{	enum {	LineMax = 10
		 ,	MaxWords = 3
		 };
	char line[LineMax+1];
	char *words[MaxWords];
 
// Open source file.
	FILE *fin = fopen("../WebSpider/unicodeLkup.txt", "r");
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


const char *unLkp_trans(csc_hash_t *ul, int ch)
{	
/*CKCK*/ fprintf(stderr, "%d ", ch);
	unLkpEnt_t *ent = csc_hash_get(ul, &ch);
	if (ent == NULL)
	{
/*CKCK*/ fprintf(stderr, "%d ", ch);
		return " ";
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




