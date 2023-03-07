csc_hash_t *readDictPath(const char *path);
// Opens 'path.  Calls readDict(). Closes path.

csc_hash_t *readDict(FILE *fin);
// Reads ASCII text version of lookup table into a hash table.  Each entry
// of the hash table is a dictionary entry as described in dictEntry_new().
// The lookup is on the lookup word only.  The hash table uses a standard
// string lookup with an offset of 1, into the dictionary entries.

char *dictEntry_new(char *lkup, char *target);
// 'lkup': Null terminated string containing the lookup entry word.
// 'target': NULL, or a null terminated string containing the substitution target.
// Returns a dictionary entry as an array of char.
// A dictionary entry consists of:-
// * ent[0]: the offset where the target begins, or 0, if (target == NULL).
// * The null terminated lookup word 'lkup' begins at ent[1].
// * If (ent[0]!=0), then null terminated 'target' begins at ent+ent[0].

void freeCharTrans();
