
// processes the body text of a document character by character.
typedef struct bodyCh_t bodyCh_t;

// Constructor.
bodyCh_t *bodyCh_new(const char *transStr);

// Destructor.
void bodyCh_free(bodyCh_t *bc);

// Set the character translation for the body.  Performs a minimal
// character translation related to the character encoding.
void bodyCh_setTrans(bodyCh_t *bc, const char *transStr);

// Send one character to the body text processor.
void bodyCh_ch(bodyCh_t *bc, int ch);
		
