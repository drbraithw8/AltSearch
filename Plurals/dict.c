
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <CscNetLib/std.h>
#include <CscNetLib/alloc.h>
#include <CscNetLib/list.h>
#include <CscNetLib/hash.h>


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
 
// Create the empty dictionary.
	csc_hash_t *dict = csc_hash_new( 1
								   , csc_hash_StrCmpr
								   , csc_hash_str
								   , csc_hash_FreeBlk
								   );
	
// Read all lines into the dictionary.
	while (csc_fgetline(fin,line,lineMax) != -1)
	{
	// Permit comment lines.
		if (line[0] == '#')
			continue;
 
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

