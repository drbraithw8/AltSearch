
typedef struct unLkpEnt_t unLkpEnt_t;
csc_hash_t *unLkp_new();
void unLkp_free(csc_hash_t *ul);
const char *unLkp_trans(csc_hash_t *ul, int ch);
int unLkp_getUtf8(FILE *fin);

