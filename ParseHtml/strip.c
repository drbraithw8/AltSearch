#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <CscNetLib/std.h>
#include <CscNetLib/alloc.h>
#include <CscNetLib/cstr.h>
#include <CscNetLib/list.h>

#define fpIn stdin   // list of 
#define foutWords stdout
#define foutUrls stdout
#define getCh() curCh = getc(fpIn)
#define skipWhite()         { while (isspace(curCh)) getCh(); }


static int curCh;

// Global Resources.
csc_str_t *curTag;
csc_str_t *curAttrib;
csc_str_t *curAttrVal;
csc_hash_t *ignoreTags;
csc_hash_t *emptyTags;


// ------------ Tag Stack ------------------------------------------

typdef struct tagStk_t
{	char *name; // Allocated.
	csc_bool_t isIgnore;
	tagStk_t *next;
} tagStk_t;


tagStk_t *tagStk_push(csc_str_t *tag, csc_bool_t isIgnore, tagStk_t *head)
{	tagStk_t *tag = csc_allocOne(tagStk_t);
	tag->name = csc_str_alloc_charr(tag);
	tag->isIgnore = isIgnore;
	tag->next = head;
	return tag;
}


tagStk_t *tagStk_pop(tagStk_t *head)
{	tagStk_t *next = head->next;
	free(head->name);
	free(head);
	return next;
}


tagStk_t *tagStk_freeMatch(tagStk_t *head)
// Unfortunately in HTML we generally do not need to provide closing tags.
// And unmatched closing tags should be ignored.
{	
// Find the matching tag.
	tagStk_t *pt;
	for (pt=head; pt!=NULL; pt=pt->next)
	{	if (csc_str_eqL(curTag, pt->name))
			break;
	}
 
// If the tag was not matched, then no need to free anything we are finished.
	if (pt == NULL)
		return head;
 
// Pop till we pop the matching tag.
	while (!csc_str_eqL(curTag, head->name))
	{	head = tagStk_pop(head);
	}
	head = tagStk_pop(head);
 
// Bye.
	return head;
}


// ------------ Tag Lookup ------------------------------------------

void setupEmptyTags()
{	const char **emptyTagStr = { "br", "input", "img", "hr", "area", "base", "col"
							   , "embed", "link", "menuitem", "param", "source"
							   , "track", "command", "wbr", "keygen", "meta", "basefont"
							   };
	emptyTags = csc_hash_new( 0   // We are passing a simple string.
						     , (int (*)(void*,void*))strcmp  /* !DANGEROUS! */
						     , csc_hash_str
						     , csc_hash_FreeNothing
						     );
	for (int i=0; i<csc_dim(emptyTagStr); i++)
	{	csc_hash_addex(emptyTags, emptyTagStr[i]);
	}
}

csc_bool_t isEmptyTag(csc_str_t *tag)
{	return csc_hash_get(emptyTags,(void*)csc_str_charr(tag)) != NULL;
}


void setupIgnoreTags()
{	const char **ignoreTagStr = { "abbr", "acronym", "applet", "area", "aside"
							   , "audio", "basefont", "button", "canvas", "code"
							   , "col", "colgroup", "data", "datalist"
							   , "del", "details", "embed", "fieldset", "form"
							   , "frame", "frameset", "head", "iframe", "kbd"
							   , "legend", "link", "map", "noframes", "object"
							   , "optgroup", "option", "output", "param", "picture"
							   , "progress", "rp", "rt", "ruby", "samp", "script"
							   , "select", "source", "style", "svg", "template"
							   , "textarea", "thead", "track", "var", "video", "wbr"
							   };
	ignoreTags = csc_hash_new( 0   // We are passing a simple string.
						     , (int (*)(void*,void*))strcmp  /* !DANGEROUS! */
						     , csc_hash_str
						     , csc_hash_FreeNothing
						     );
	for (int i=0; i<csc_dim(ignoreTagStr); i++)
	{	csc_hash_addex(ignoreTags, ignoreTagStr[i]);
	}
}

csc_bool_t isIgnoreTag(csc_str_t *tag)
{	return csc_hash_get(ignoreTags, void*)csc_str_charr(tag)) != NULL;
}
	

// Devours a HTML comment.  Assumes that we are on the opening "!" of a
// HTML comment.  We expect the next two characters to be "--".  Its an
// error if not.
void eatComment()
{	int ch1 = ' ';
	int ch2 = ' ';
 
// Eat first '-'.
	if (curCh == '-')
	{	getCh();
	}
	else
	{	// Its an error.
	}
		
// Eat second '-'.
	if (curCh == '-')
	{	getCh();
	}
	else
	{	// Its an error.
	}
 
// Skip to end of comment.
	for (;;)
	{	if (curCh==EOF || (curCh=='>' && ch1=='-' && ch2=='-'))
			break;
		ch2 = ch1;
		ch1 = curCh;
		getCh();
	}
 
// Read past last character of comment.
	if (ch != EOF)
		getCh();
}


void readIdent(csc_str_t *ident)
{	csc_str_reset(ident);
	while (isalnum(curCh) || curCh=='_')
	{	csc_str_append_ch(idName, curCh);
		getCh();
	}
}
							 	

void quotedAttrib(csc_str_t *attrVal)
// Reads a quoted attribute.
// Assumes that 'curCh' contains the opening quote character.
{	int qCh = curCh;
	assert(qCh=='\'' || qCh=='"');
 
// Read past quote.
	getCh();
 
// Read in the attribute.
	csc_str_reset(curAttrVal);
	while (curCh != qCh)
	{	csc_str_append_ch(attrVal, curCh); \
		getCh();
	}
 
// Read past quote.
	getCh();
}


int readTag()
// Reads in a HTML tag.  Returns 1 for an opening tag, -1 for a closing
// tag, and 0 for a tag that is complete, such as <br> or <aa/> or a
// comment.  Assigns the tag name to curTag.
// Assumes that 'curCh' contains the opening "<" of the tag.
{	int openClose = 1;
	csc_bool_t isUrl;
	
// Read past the opening "<".
	assert(curCh == '<');
	getCh();
 
// This might be a comment.
	if (curCh == '!')
	{	eatComment();
		return 0;
	}
 
// Devour whitespace.
	skipWhite();
 
// Is it a closing tag.
	if (curCh == '/')
	{	openClose = -1;
		getCh();
 
	// Devour whitespace.
		skipWhite();
	}
 
// Read tag name.
	assert(isalpha(curCh));   // TODO: Bailing out is inappropriate.
	readIdent(curTag);
	if (isEmptyTag(curTag) && openClose==1)
		openClose = 0;
 
// Read attributes.
	while (curCh != '>')
	{	skipWhite();
 
	// Read the attribute.
		if (isalpha(curCh))
		{
		// Attribute name.
			readIdent(curAttrib);
 
		// Expecting '='.
			skipWhite();
			if (curCh == '=')
			{	getCh();
 
			// Attribute value.
				if (isalpha(curCh))
				{	readIdent(curAttrVal);
				}
				else if (curCh=='"' || curCh=='\'')
				{
				// We have a curTag, and a quoted attribute.
					quotedAttrib(curAttrVal);
 
				// Print it out if we think its a link.
					if (csc_str_eqL(curAttrib,"href"))
					{	char *attVal = csc_str_charr(curAttrVal);
						if (csc_str_eqL(curTag,"a"))
							fprintf(foutUrls, "a %s\n", attVal);
						else if (csc_str_eqL(curTag,"base"))
							fprintf(foutUrls, "b %s\n", attVal);
					}
				}
			}
			else
				getCh();
		}
		else if (curCh == '/')
		{	getCh();
			skipWhite();
			if (curCh=='>' && openClose==1)
				openClose = 0;
		}
		else
			getCh();
	}
 
// Read past the final '>'.
	getCh();
 
// Bye.
	return openClose;
}


void processWord(char *str)
{	fprintf(foutWords, "%s\n", str);
}


int main(int argc, char **argv)
{	csc_list_t *tagStack = NULL;	
	csc_bool_t isIgnore;
	csc_str_t *word;
	int openClose;
	tag_t *tag;
 
// Initialise resources.
	word = csc_str_new();
	curTag = csc_str_new();
	curAttrib = csc_str_new();
	curAttrVal = csc_str_new();
	setupIgnoreTags();
	setupEmpty();
 
// Read through the document.
	getCh();
	while (curCh != EOF)
	{	if (curCh == '<')
		{	openClose = readTag();
			if (openClose == 1)			// Its an opening tag.
			{	if (!isIgnore)
				{	isIgnore = isIgnoreTag(curTag);
				}
				tagStack = tagStk_push(curTag, isIgnore, tagStack);
			}
			else if (openClose == -1)	// Its a closing tag.
			{	tagStack = tagStk_freeMatch(head);  // Pop to matching tag.
				isIgnore = head->isIgnore;
			}
			else	// Its an empty tag.
			{	
			}
		}
		else if (!isIgnore && isalpha(curCh))
		{	readIdent(word);
			if (csc_str_length(word)>1)
				processWord(word);
		}
		else
			getCh();
	}
	
// Free resources.
	csc_str_free(word);
	csc_str_free(curTag);
	csc_str_free(curAttrib);
	csc_str_free(curAttrVal);
	csc_hash_free(emptyTags);
	csc_hash_free(ignoreTags);
	return 0;
}

