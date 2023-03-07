
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <CscNetLib/std.h>
#include <CscNetLib/alloc.h>
#include <CscNetLib/list.h>
#include <CscNetLib/hash.h>

#include "dict.h"


char *progName;
void usage()
{	fprintf(stderr, "Usage: %s dictPath britPath outPath\n", progName);
	exit(1);
}


static csc_list_t *readEntList(const char *path)
{	enum { lineMax=40 };
	char line[lineMax+1];
	csc_list_t *list = NULL;
 
	FILE *fin = fopen(path, "r");
	if (fin == NULL)
	{	fprintf( stderr
			   , "Error: Could not open file \"%s\" for read!\n"
			   , path
			   );
		usage();
	}
 
// Read all lines into the dictionary.
	while (csc_fgetline(fin,line,lineMax) != -1)
	{	char *words[3];
		char *ent;
 
	// Permit comment lines.
		if (line[0] == '#')
			continue;
 
	// Split line into words.
		int nWds = csc_param(words, line, 3);
 
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
 
 	// Add the entry to the list.
		csc_list_add(&list, ent);
	}
	fclose(fin);
 
	csc_list_rvrse(&list);
	return list;
}	


// For each word in 'dict', if replacing a 'srch' at the end of word X with
// 'repl' makes a word Y in 'brit' that does not exist in 'dict' then add a
// substitution entry Y -> X to X in 'dict'.
static void britEndSubs( csc_hash_t *dict, csc_hash_t *brit
					   , const char *srch, const char *repl
					   )
{	char *entX;
	char *x;
	char y[101];
 
// Make new empty list.
	csc_list_t *newEnts = NULL;

// Search and make new entries.
	csc_hash_iter_t *iter = csc_hash_iter_new(dict);
	while ((entX=csc_hash_iter_next(iter)) != NULL)
	{	char *x = entX +1;
 
	// Does 'srch' exist at end of  'X'?
		int lenS = strlen(srch);
		int lenX = strlen(x);
		if (lenX<=lenS || strcmp(srch,x+lenX-lenS))
			continue;
 
	// Y = X with 'srch' replaced by 'repl'.
		csc_strncpy(y, x, lenX-lenS);
		strcat(y, repl);
 
	// Does 'Y' exist in 'brit'?
		if (csc_hash_get(brit,y) == NULL)
			continue;
 
	// New entry entY(Y).  Add to list.
		csc_list_add(&newEnts, dictEntry_new(y, x));
	}
	csc_hash_iter_free(iter);
 
// Add in the new entries.unicodeLkup.txt
	for (csc_list_t *p=newEnts; p!=NULL; p=p->next)
	{	entX = p->data;
		if (!csc_hash_addex(dict, entX))
		{	free(entX);
		}
		else
		{	// fprintf(stdout, "%s %s\n", entX+1, entX+entX[0]);
		}
	}
 
// Free list.
	csc_list_free(newEnts);
}


// For each word in 'dict', if replacing a 'srch' at a place within word X
// with 'repl' makes a word Y in 'brit' that does not exist in 'dict' then
// add a substitution entry Y -> X to X in 'dict'. 
static void britInSubs( csc_hash_t *dict, csc_hash_t *brit
					  , const char *srch, const char *repl
					  )
{	char y[101];
	int lenS = strlen(srch);
	char *entX;
 
// Make new empty list.
	csc_list_t *newEnts = NULL;
 
// Search and make new entries.
	csc_hash_iter_t *iter = csc_hash_iter_new(dict);
	while ((entX=csc_hash_iter_next(iter)) != NULL)
	{	char *x = entX + 1;
		for (char *p=strstr(x,srch); p!=NULL; p=strstr(p+1,srch))
		{
		// Y = X with 'srch' replaced by 'repl'.
			csc_strncpy(y, x, p-x);
			strcat(y, repl);
			strcat(y, p+lenS);
 
		// Does 'Y' exist in 'dict'?
			if (csc_hash_get(dict,y) != NULL)
				continue;
 
		// Does 'Y' exist in 'brit'?
			if (csc_hash_get(brit,y) == NULL)
				continue;
 
		// New entry entY(Y).  Add to dict.
			csc_list_add(&newEnts, dictEntry_new(y,x));
			break;
		}
	}
	csc_hash_iter_free(iter);
 
// Add in the new entries.
	for (csc_list_t *p=newEnts; p!=NULL; p=p->next)
	{	entX = p->data;
		if (!csc_hash_addex(dict, entX))
		{	free(entX);
		}
		else
		{	// fprintf(stdout, "%s %s\n", entX+1, entX+entX[0]);
		}
	}
 
// Free list.
	csc_list_free(newEnts);
}


// For all words X in Dict that do not have a substitution, if X ends in
// 'srch', and replacing it with 'repl' makes a word Y then add a
// substitution entry Y -> X to X in Dict. 
static void dictEndSubs(csc_hash_t *dict, const char *srch, const char *repl)
{	char y[101];
	int lenS = strlen(srch);
	char *entX;
 
// Make new empty list.
	csc_list_t *newEnts = NULL;
 
// Search and make new entries.
	csc_hash_iter_t *iter = csc_hash_iter_new(dict);
	while ((entX=csc_hash_iter_next(iter)) != NULL)
	{	
	// Does this entry does already have a substitution?
		if (*entX != 0)
			continue;
 
	// Does 'srch' exist at end of  'X'?
		char *x = entX +1;
		int lenS = strlen(srch);
		int lenX = strlen(x);
		if (lenX<=lenS || strcmp(srch,x+lenX-lenS))
			continue;
 
	// Y = X with 'srch' replaced by 'repl'.
		csc_strncpy(y, x, lenX-lenS);
		strcat(y, repl);
 
	// Does 'Y' exist in 'dict'?
		if (csc_hash_get(dict,y) == NULL)
			continue;
 
	// New substitution entry entY(Y->X).  Add to list.
		csc_list_add(&newEnts, dictEntry_new(x, y));
	}
	csc_hash_iter_free(iter);
 
// Add substitution entries to dict.
	for (csc_list_t *p=newEnts; p!=NULL; p=p->next)
	{	char *newEntX = p->data;
 
	// Remove old entry.
		char *oldEntX = csc_hash_out(dict, newEntX+1);
		csc_assert(oldEntX != NULL);
		free(oldEntX);
 
	// Add in new substitution entry.
		csc_bool_t ret = csc_hash_addex(dict, newEntX);
		csc_assert(ret);
		// fprintf(stdout, "%s %s\n", newEntX+1, newEntX+newEntX[0]);
	}
 
// Free list.
	csc_list_free(newEnts);
}


// It is possible that word A has a substitution entry such that A points
// to word B, and when we look at word B we find that it points to word C,
// which points to word D.  This function will change the dictionary
// 'dict' so that A, B and C all point to D.
static void unrollSubsts(csc_hash_t *dict)
{	char *entX;
 
// Make new empty list.
	csc_list_t *newEnts = NULL;
 
// Search and make new entries.
	csc_hash_iter_t *iter = csc_hash_iter_new(dict);
	while ((entX=csc_hash_iter_next(iter)) != NULL)
	{	
	// Does this entry does already have a substitution?
		int indX = *entX;
		if (indX == 0)
			continue;
		
	// Find the final substitution 'f'.
		char *y = entX + indX;
		char *s = y;
		char *f = NULL;
		while (f == NULL)
		{	char *entS = csc_hash_get(dict,s);
			if (entS == NULL)
			{	fprintf( stderr
					   , "Error: Target \"%s\" for \"%s\" does not exist!\n"
					   , s, entX+1
					   );
				f = s;
			}
 			else
			{	int indS = *entS;
				if (indS == 0)
					f = s;
				else
					s = entS + indS;
			}
		}
 
	// Is 'f' different from 'y'?
		if (csc_streq(f,y))
			continue;
 
	// New substitution entry entY(X->F).  Add to list.
		csc_list_add(&newEnts, dictEntry_new(entX+1, f));
	}
	csc_hash_iter_free(iter);
 
// Add substitution entries to dict.
	for (csc_list_t *p=newEnts; p!=NULL; p=p->next)
	{	char *newEntX = p->data;
 
	// Remove old entry.
		char *oldEntX = csc_hash_out(dict, newEntX+1);
		csc_assert(oldEntX != NULL);
		free(oldEntX);
 
	// Add in new substitution entry.
		csc_bool_t ret = csc_hash_addex(dict, newEntX);
		csc_assert(ret);
		// fprintf(stdout, "%s %s\n", newEntX+1, newEntX+newEntX[0]);
	}
 
// Free list.
	csc_list_free(newEnts);
}


// Adds new word entries to the dictionary, or replace existing entries. The
// file 'fpath' contains words one per line, and each line possibly has a
// substitution entry.  For each of these words:- If they exist in 'dict',
// then they will be replaced by the word (and the substitution entry, if
// it exists).  Words not already existing in 'dict' will be added with the
// substitution entry if it exists.  
static void dict_addReplace(csc_hash_t *dict, char *fPath)
{	char *entDict, *entNewWds, *entNew;
 
// Read in the new words.
	csc_list_t *wordList = readEntList(fPath);
 
// Loop through entries in newWords.
	for (csc_list_t *lw=wordList; lw!=NULL; lw=lw->next)
	{	entNewWds = lw->data;
		char *word = entNewWds+1;
 
	// Remove any such entry from 'dict'.
		entDict = csc_hash_out(dict, word);
		if (entDict != NULL)
			free(entDict);
 
	// Copy the new entry, and insert it into 'dict.
		if (*entNewWds == 0)
		{	entNew = dictEntry_new(word, NULL);
			csc_hash_addex(dict, entNew);
		}
		else
		{	char *subst = entNewWds+(*entNewWds);
			if (csc_hash_get(dict,subst) == NULL)
			{	fprintf( stderr
					   , "Error: subst \"%s\" does not already exist in dict!\n"
					   , subst
					   );
			}
			else
			{	entNew = dictEntry_new(word, entNewWds+(*entNewWds));
				csc_hash_addex(dict, entNew);
			}
		}
	}
 
// Free the new words.
	csc_list_freeblk(wordList);
}


// Remove entries in 'dict' matching words found in file 'fPath'.
static void dict_remove(csc_hash_t *dict, char *fPath)
{	char *entDict, *entNewWds;
 
// Read in the new words.
	csc_list_t *wordList = readEntList(fPath);
 
// Remove any such entry from 'dict'.
	for (csc_list_t *lw=wordList; lw!=NULL; lw=lw->next)
	{	char *entNewWds = lw->data;
		entDict = csc_hash_out(dict, entNewWds+1);
		if (entDict != NULL)
			free(entDict);
	}
 
// Free the new words.
	csc_list_freeblk(wordList);
}


void main(int argc, char **argv)
{	progName = argv[0];
 
// Does this look plausible?
	if (argc != 4)
	{	usage();	
	}
 
// Open output.
	FILE *fout = fopen(argv[3], "w");
	if (fout == NULL)
	{	fprintf( stderr
			   , "Error: Could not open file \"%s\" for write!\n"
			   , argv[3]
			   );
		usage();
	}
 
// Read in dictionary.
	csc_hash_t *dict = readDictPath(argv[1]);
	if (dict == NULL)
	{	fprintf( stderr
			   , "Error: Could not open file \"%s\" for read!\n"
			   , argv[1]
			   );
		usage();
	}
 
// Read in British dictionary.
	csc_hash_t *brit = readDictPath(argv[2]);
	if (dict == NULL)
	{	fprintf( stderr
			   , "Error: Could not open file \"%s\" for read!\n"
			   , argv[2]
			   );
		usage();
	} 
 
// Operations
	britEndSubs(dict, brit, "ize", "ise");
	britEndSubs(dict, brit, "er", "re");
	britInSubs(dict, brit, "ai", "ae");
	britInSubs(dict, brit, "ne", "gn");
	britInSubs(dict, brit, "ov", "oov");
	britInSubs(dict, brit, "to", "tte");
	britInSubs(dict, brit, "a", "ae");
	britInSubs(dict, brit, "o", "ou");
	britInSubs(dict, brit, "e", "oe");
	britInSubs(dict, brit, "l", "ll");
	britInSubs(dict, brit, "e", "");
	dictEndSubs(dict, "tion", "te");
	dictEndSubs(dict, "ies", "");
	dictEndSubs(dict, "d", "");
	dictEndSubs(dict, "ed", "");
	dictEndSubs(dict, "s", "");
	dictEndSubs(dict, "es", "");
	dictEndSubs(dict, "'s", "");
 
// Dont process ".ing", because it gives wrong meanings.
// 	dictEndSubs(dict, "ing", "");  // ship!=shipping box!=boxing
 
// Add in any new words and their substitutions.
	dict_addReplace(dict, "addReplace.txt");
	dict_remove(dict, "remove.txt");
 
// Unroll multiple indirects.
	unrollSubsts(dict);
 
// Write out dictionary.
	csc_hash_iter_t *iter = csc_hash_iter_new(dict);
	char *ent;
	while ((ent=csc_hash_iter_next(iter)))
	{	fprintf(fout, "%s", ent+1);
		if (ent[0] == 0)
		{	fprintf(fout, "\n");
		}
		else
		{	fprintf(fout, " %s\n", ent + ent[0]);
		}
	}
	csc_hash_iter_free(iter);
 
// Free dictionaries.
	csc_hash_free(dict);
	csc_hash_free(brit);

// Free UTF8 lookup.
	freeCharTrans();
 
// Bye.
	fclose(fout);
	csc_mck_print(stderr);
	exit(0);
}
 
