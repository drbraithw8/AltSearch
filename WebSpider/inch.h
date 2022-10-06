
// ----------------------- inch_t ---------------------------------------

// Type for getting an ASCII character.
// *	Provides n character lookahead, with n in the range 1-4.
typedef struct inch_t inch_t;


// Constructor.  See inch_setGetAsc() for details of 2nd and 3rd arg.
inch_t *inch_new(FILE *fin);

// Destructor.
void inch_free(inch_t *inch);

// Get one char.  EOF indicated by -1.
int inch_curCh(inch_t *inch);

// Moves the buffer by one character.  Returns the current character.
int inch_next(inch_t *inch);

// Lookahead n chars.  Writes null terminated string into buf, which must
// have size 5.  Null terminated string may be short as we approach the EOF.
// returns argument 'buf'.
char *inch_lookAhead(inch_t *inch, char buf[5]);

// *	getAscChar() is a function for getting the char, i.e. translating to
// 	ASCII from:-
// 	*	windows-1252
// 	*	windows-1252
// 	*	UTF8
// 	*	ASCII (no-op)

csc_bool_t inch_set_getAscChar(inch_t *inch, const char *charset);


// Debugging.
void inch_reportPos(inch_t *inch, FILE *fout);

// ----------------------------------------------------

#define CKCK(ii)  ( fprintf(stderr, "%s %d: ", __FILE__, __LINE__), \
                          inch_reportPos(ii,stderr) )
