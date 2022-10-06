
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <CscNetLib/std.h>
#include <CscNetLib/alloc.h>
#include <CscNetLib/hash.h>
#include "inch.h"
#include "bodyChar.h"


// -------------------------------------------------------

typedef struct url_t
{	char *url;
	char *txt;
} url_t;


url_t *url_new(const char *url, const char *txt)
{	url_t *u = csc_allocOne(url_t);
	u->url = csc_alloc_str(url);
	if (txt==NULL || csc_streq(txt,""))
		u->txt = NULL;
	else
		u->txt = csc_alloc_str(txt);
	return u;
}


void url_free(url_t *u)
{	free(u->url);
	if (u->txt)
		free(u->txt);
	free(u);
}


// -------------------------------------------------------

enum {	procHtml_titleMax = 200
	 ,	procHtml_charsetMax = 15
	 };
typedef struct procHtml_t
{	inch_t *inch;
    char title[procHtml_titleMax+1];
    char *baseUrl;
    char *thisUrl;
    char explicitCharset[procHtml_charsetMax+1];
    char defaultCharset[procHtml_charsetMax+1];
    csc_hash_t *urls;
    csc_hash_t *words;
	bodyCh_t *bc;
} procHtml_t;

void set_charset(procHtml_t *ph, const char *charset, csc_bool_t isExplicit);


procHtml_t *procHtml_new( inch_t *inch
                          , const char *baseUrl
                          , const char *thisUrl
                          , const char *defaultCharset
                        )
{
// Allocate the structure.
    procHtml_t *ph = csc_allocOne(procHtml_t);
 
// Character input.
    ph->inch = inch;
 
// Default strings.
    strcpy(ph->title, "");
    ph->baseUrl = csc_alloc_str(baseUrl);
    ph->thisUrl = csc_alloc_str(thisUrl);
 
// If there is a default charset, then set it.
    strcpy(ph->explicitCharset, "");
    strcpy(ph->defaultCharset, "");
    if (strcmp(defaultCharset, ""))
    {   set_charset(ph, defaultCharset, csc_FALSE);
    }
 
// Hash tables for words and URLs.
    ph->urls  = csc_hash_new( 0
                              , csc_hash_StrPtCmpr
                              , csc_hash_StrPt
                              , (void(*)(void*))url_free
                            );
    ph->words = csc_hash_new( 0
                              , csc_hash_StrCmpr
                              , csc_hash_str
                              , free
                            );
 
// BodyChars.
	ph->bc = bodyCh_new("ASCII");
	
// Deliver the goods.
    return ph;
}


void procHtml_free(procHtml_t *ph)
{   free(ph->baseUrl);
    free(ph->thisUrl);
    csc_hash_free(ph->words);
    csc_hash_free(ph->urls);
	bodyCh_free(ph->bc);
    free(ph);
}

// -------------------------------------------------
 
int skipWhiteSpace(inch_t *inch)
{   int ch = inch_curCh(inch);
    while (isspace(ch))
    {   ch = inch_next(inch);
    }
    return ch;
}


int skipComment(inch_t *inch)
{   int ch;
    char buf[4];
    csc_bool_t isDone = csc_FALSE;
 
// Skip "<!--".
    ch = inch_next(inch);
 
// Find "-->".
    while (!isDone)
    {   while (ch!=-1 && ch!='-')
        {   ch = inch_next(inch);
        }
        if (ch == -1)
        {  
			isDone = csc_TRUE;
        }
        else
        {   inch_lookAhead(inch, buf);
            buf[3] = '\0';
            if (csc_streq(buf, "-->"))
            {   isDone = csc_TRUE;
            }
            else
            {   ch = inch_next(inch);
            }
        }
    }
 
// Skip "-->".
    inch_next(inch);
    inch_next(inch);
 
// Return the next character.
    return inch_next(inch);
}


// Reads a quote.  The quote character, the current character in 'inch', is
// also used to terminate the quote.  A character escaped with a "\\" is
// taken literally.
//
// If 'wd' is not NULL, then the word will be read into 'wd'.  If the word
// is longer than 'wMax' characters, then only the first 'wMax' chars of the
// line will be placed into 'wd'.  The remainder of the word will be
// skipped.  This function will append a '\0' to the characters read into
// 'wd'.  Hence 'wd' should have room for 'wMax'+1 characters.
int readQuotStr(inch_t *inch, char *wd, int wMax)
{   int quotChar = inch_curCh(inch);
    int ch;
 
// Measure to stop any writing into 'wd' if it is NULL.
    if (wd == NULL)
    {   wMax = 0;
    }
 
// Eat the string.
    int iWd = 0;
    ch = inch_next(inch);
    while (ch!=-1 && ch!=quotChar)
    {   if (ch == '\\')
        {   ch = inch_next(inch);  // Read past "\\".
        }
        if (iWd < wMax)
        {   wd[iWd++] = ch;
        }
        ch = inch_next(inch);
    }
 
// Eat the terminating quote character.
    ch = inch_next(inch);
 
// Terminate the string.
    if (wd != NULL)
    {   wd[iWd] = '\0';
    }
 
// Return the next character.
    return ch;
}


// We are expecting a tag name (opening or closing), possibly preceded
// by whitespace.  XML element names contain only letters, digits,
// hyphens, underscores, and periods.
//
// If 'wd' is not NULL, then the tag will be read into 'wd'.  If the word
// is longer than 'wMax' characters, then only the first 'wMax' chars of the
// line will be placed into 'wd'.  The remainder of the word will be
// skipped.  This function will append a '\0' to the characters read into
// 'wd'.  Hence 'wd' should have room for 'wMax'+1 characters.
int readTagName(inch_t *inch, char *wd, int wMax)
{
// Measure to stop any writing into 'wd' if it is NULL.
    if (wd == NULL)
    {   wMax = 0;
    }
 
// Skip whitespace.
    skipWhiteSpace(inch);
 
// Eat the word.
    int iWd = 0;
    int ch = skipWhiteSpace(inch);
    while (isalnum(ch) || ch=='-' || ch=='_' || ch=='.')
    {   if (iWd < wMax)
        {   wd[iWd++] = ch;
        }
        ch = inch_next(inch);
    }
 
// Terminate the string.
    if (wd != NULL)
    {   wd[iWd] = '\0';
    }
 
// Return the next character.
    return ch;
}


// This reads past a tag and returns the following character.
int skipTag(inch_t *inch)
{
// Read until we find the ">".
    int ch = inch_curCh(inch);
    csc_bool_t isDone = csc_FALSE;
    while (ch!=-1 && ch!='>')
    {   if (ch=='"' || ch=='\'')
        {   ch = readQuotStr(inch, NULL, 0);
        }
        else
        {   ch = inch_next(inch);
        }
    }
 
// Return the next character.
    return inch_next(inch);
}


// We have just read past the opening tag name of 'tagOpen', The current character
// may or may not be the closing angular bracket of the opening HTML tag.
// 'tagOpen' might be a "script" tag or it might be something else that can
// be treated in a similar way.
//
// This function reads through until the end of a closing tag of 'tagOpen',
// eating the closing angular bracket of the closing HTML tag.
int eatJscript(inch_t *inch, char *tagOpen)
{   enum {TAG_MAX = 20};
    char tag[TAG_MAX+1];
    int ch;
 
    ch = skipTag(inch);
    csc_bool_t isDone = csc_FALSE;
    while (!isDone)
    {   if (ch=='"' || ch=='\'')
        { 
			ch = readQuotStr(inch, NULL, 0);
        }
        else if (ch == '<')
        { 
			inch_lookAhead(inch, tag);
            inch_next(inch);
            ch = skipWhiteSpace(inch);
            if (csc_streq(tag, "<!--"))
            { 
				ch = skipComment(inch);
            }
            else if (ch == '/')
            { 
				inch_next(inch);
				ch = readTagName(inch, tag, TAG_MAX);
                if (csc_strieq(tagOpen, tag))
                {  
					isDone = csc_TRUE;
					ch = skipTag(inch);
                }
            }
        }
        else if (ch == -1)
        {   isDone = csc_TRUE;
        }
        else
        {   ch = inch_next(inch);
        }
    }
 
// Return the next character.
    return ch;
}


// We found a string within a meta tag.  This function looks for
// "charset=????", and if it finds that, it assigns the value to 'charset'.
// The current character is the opening quote character of the string.
int readMetaStr(inch_t *inch, char *charname, char *charset, int charsetMax)
{   int quotChar = inch_curCh(inch);
 
// Holds a field name.
    enum {nameMax = 10};
    char name[nameMax+1];
    strcpy(name,"");
 
// Holds a field value.
    enum {valMax = 12};
    char val[valMax+1];
    strcpy(val,"");
 
// Look for next interesting thing.
    int ch = inch_next(inch);
    while (!isalpha(ch) && ch!=quotChar && ch!=-1)
    {   ch = inch_next(inch);
    }
 
    while (ch!=quotChar && ch!=-1)
    {  
	// We found an identifier.  Read it in and skip trailing spaces.
        int iName = 0;
        while (isalnum(ch) || ch=='-')
        {   if (iName < nameMax)
            {   name[iName++] = ch;
            }
            ch = inch_next(inch);
        }
        name[iName] = '\0';
        ch = skipWhiteSpace(inch);
 
	// Did we find an "=".
        if (ch == '=')
        {
		// Read past the "=", and past any whitespace.
            ch = inch_next(inch);
            ch = skipWhiteSpace(inch);
 
		// Read a value.
            int iVal = 0;
            while(!isspace(ch) && ch!=quotChar && ch!=-1)
            {   if (iVal < valMax)
                {   val[iVal++] = ch;
                }
                ch = inch_next(inch);
            }
            val[iVal] = '\0';
 
		// Look at our name/value pair.
            if (csc_strieq(name,"charset"))
            {   csc_strncpy(charset, val, charsetMax);
            }
        }
        else if (csc_strieq(charname, "charset"))
        {   csc_strncpy(charset, name, charsetMax);
        }
 
	// Look for next interesting thing.
        while (!isalpha(ch) && ch!=quotChar && ch!=-1)
        {   ch = inch_next(inch);
        }
    }
 
// Read past the quote, and return the following char.
    return inch_next(inch);
}


void set_charset(procHtml_t *ph, const char *charset, csc_bool_t isExplicit)
{  
// Dont do it, if the default or explicit charset is already set.
	if (strcmp(ph->explicitCharset,""))
	{	fprintf( stderr
		       , "Cant set charset to \"%s\".  Already set explicitly.\n"
			   , charset
			   );
		return;
	}
    else if (!isExplicit && strcmp(ph->defaultCharset,""))
	{	fprintf( stderr
		       , "Cant set default charset to \"%s\".  Default already set.\n"
			   , charset
			   );
		return;
	}

// Try to set the character set.
	if (inch_set_getAscChar(ph->inch, charset))
	{	if (isExplicit)
		{   fprintf(stderr, "Explicit set charset to \"%s\".\n", charset);
			strcpy(ph->explicitCharset,charset);
		}
		else
		{   fprintf(stderr, "Default set charset to \"%s\".\n", charset);
			strcpy(ph->defaultCharset,charset);
		}
	}
	else
	{	fprintf(stderr, "Failed to set unknown charset \"%s\".\n", charset);
	}
}


// Have read in the opening "<" and the tag name "meta".  This function
// reads the rest of the meta tag, and if it specifies the charset, then
// this function will assign the charset string to 'charset'.
int readMeta(inch_t *inch, char *charset, int charsetMax)
{
// Holds a field name.
    enum {nameMax = 10};
    char name[nameMax+1];
    strcpy(name,"");
 
// Holds a value value.
    enum {valMax = 20};
    char val[valMax+1];
    strcpy(val,"");
 
// No charset yet.
    strcpy(charset,"");
 
// Read up to the next interesting thing.
    int ch = inch_curCh(inch);
    while (!isalpha(ch) && ch!='>' && ch!=-1)
    {   ch = inch_next(inch);
    }
 
    while (isalpha(ch))
    {   // Read the tag name and skip trailing spaces.
        readTagName(inch, name, nameMax);
        ch = skipWhiteSpace(inch);
 
        // Did we find an "=".
        if (ch == '=')
        {  
		// Read past the "=", and past any whitespace.
            inch_next(inch);
            ch = skipWhiteSpace(inch);
 
		// Is it a quoted value?
            if (ch=='"' || ch=='\'')
            {   ch = readMetaStr(inch, name, charset, charsetMax);
            }
            else
            {   int iVal = 0;
                while (!isspace(ch) && ch!='>' && ch!=-1)
                {   if (iVal < valMax)
                    {   val[iVal++] = ch;
                    }
                    ch = inch_next(inch);
                }
                val[iVal] = '\0';
 
			// Look at our name/value pair.
                if (csc_strieq(name,"charset"))
                {   csc_strncpy(charset, val, charsetMax);
                }
            }
 
		// Move to the next name.
            ch = skipWhiteSpace(inch);
        }
 
	// Read up to the next interesting thing.
        while (!isalpha(ch) && ch!='>' && ch!=-1)
        {   ch = inch_next(inch);
        }
    }
 
// Read past the '>', and return the following char.
    return inch_next(inch);
}


// Read a "title" (or "h1" tag etc) and extract the inner html into 'title'
// if that is not NULL.  The string "title" has been read in and the
// current character is whatever terminates the string "title".  The
// character after the closing ">" of the closing "title" tag will be
// returned.
int htmlTagTitle(inch_t *inch, char *title, int titleMax)
{
// Do we read in a title?
    if (title == NULL)
        titleMax = 0;
 
// Read in the title.
    int iTitle = 0;
    int ch = skipTag(inch);
    while (ch!='<' && ch!=-1)
    {   if (iTitle<titleMax && isprint(ch))
		{	title[iTitle++] = ch;
        }
        ch = inch_next(inch);
    }
 
// Terminate the title.
    if (title != NULL)
    {   title[iTitle] = '\0';
    }
 
// Skip the closing tag.
    return skipTag(inch);
}


// Read an "a" tag and extracts the value of a possible "href".
// If there is "href" field then it will be read into 'href'.
// Otherwise 'href' will be set to the empty string.
//
// The "a" has been read in and the current character is whatever
// terminates the string "a".
int htmlTagA( procHtml_t *ph
			, char *href, int hrefMax
			, char *txt, int txtMax
			)
{	inch_t *inch = ph->inch;
 
// Holds a field name.
    enum {fldNameMax = 10};
    char fldName[fldNameMax+1];
    strcpy(fldName,"");
 
// We did not find anything yet.
    strcpy(href,"");
    strcpy(txt,"");
 
// Find what looks like an identifier or the end of the tag.
    int ch = inch_curCh(inch);
    while (!isalpha(ch) && ch!='>' && ch!=-1)
    {   ch = inch_next(inch);
    }
 
// Read in fields.
    csc_bool_t isHref = csc_FALSE;
    while (isalpha(ch))
    {   csc_bool_t isHref = csc_FALSE;
 
        // Read the field name.  Was it "href"?
        ch = readTagName(inch, fldName, fldNameMax);
        if (csc_strieq(fldName, "href"))
        {   isHref = csc_TRUE;
        }
 
        // Is the field assigned a value?
        ch = skipWhiteSpace(inch);
        if (ch == '=')
        { 
		// Read past the "=".
            ch = inch_next(inch);
            ch = skipWhiteSpace(inch);
 
		// Is the field value a quoted string?
            if (ch=='"' || ch=='\'')
            {   if (isHref)
                    ch = readQuotStr(inch, href, hrefMax);
                else
                    ch = readQuotStr(inch, NULL, 0);
            }
            else
            {   int dMax = 0;
 
			// If field name is a "href=" then read into 'href'.
                if (isHref)
                    dMax = hrefMax;
 
			// Read in, or read past the field value.
                int iData=0;
                while (!isspace(ch) && ch!='>' && ch!=-1)
                {   if (iData < dMax)
                    {   href[iData++] = ch;
                    }
                    ch = inch_next(inch);
                }
 
			// Terminate the href string.
                if (isHref)
                    href[iData] = '\0';
            }
        }
 
	// Find what looks like the next identifier or the end of the tag.
        while (!isalpha(ch) && ch!='>' && ch!=-1)
        {   ch = inch_next(inch);
        }
    }
 
// The current character should be '>'.  Read past it.
    ch = inch_next(inch);
 
// Read in plain text.
// /*TODO*/  Does not currently read in plain text of a link if that
// text is within <i>, <b>, <em>, <font> etc.  Seems like a big job to
// get this right.  How far to go with this?  Fix this if it turns out
// that we actually use the text.
	if (csc_streq(href,""))
	{	return ch;
	}
	else
	{	int iTxt = 0;
		int lastCh = ' ';
		while (ch!='<' && ch!=-1)
		{	if (iTxt < txtMax)
			{	if (isspace(ch))
				{	ch = ' ';
					if (lastCh != ch)
					{	txt[iTxt++] = ch;
					}
				}
				else if (isprint(ch))
				{	txt[iTxt++] = ch;
				}
				lastCh = ch;
			}
			// bodyCh_ch(ph->bc, ch);
			ch = inch_next(inch);
		}
		txt[iTxt] = '\0';
		csc_trim(txt);
		return skipTag(inch);
	}
}


// This reads a special tag, and if it is "DOCTYPE html", will set the
// character set to "UTF8" if a character set has not been specified.  The
// current character is "!".
int readSpecialTag(inch_t *inch, char *charset, int charsetMax)
{
// Holds a tag name
    enum {tagMax=12};
    char tag[tagMax+1];
    strcpy(tag,"");
 
// Holds a value.
    enum {valMax = 10};
    char val[valMax+1];
    strcpy(val,"");
 
// Read past "!", and skip spaces.
    inch_next(inch);
    int ch = skipWhiteSpace(inch);
 
// Read tag, and skip spaces.
    if (!isalpha(ch))
    {   return skipTag(inch);
    }
    readTagName(inch, tag, tagMax);
    ch = skipWhiteSpace(inch);
 
// Read value and skip to end of tag.
    if (!isalpha(ch))
    {   return skipTag(inch);
    }
    readTagName(inch, val, valMax);
    ch = skipTag(inch);
 
// If we have "DOCTYPE HTML", then set the default charset to "UTF8".
    if (csc_strieq(tag,"doctype") && csc_strieq(val,"html"))
    {   csc_strncpy(charset, val, charsetMax);
    }
 
// Return the next character.
    return ch;
}


// This reads past the opeing tag, reads the title into 'title' and also
// reads the closing "title" tag.  The current character is the one after
// the tag name "title".
int readTitle(inch_t *inch, char *title, int titleMax)
{   skipTag(inch);
 
// Read in the title.
    int ch = skipWhiteSpace(inch);
    int iTitle = 0;
    while (ch!='<' && ch!=-1)
    {   if (iTitle < titleMax)
        {   if (isspace(ch))
            {   if (iTitle==0 || title[iTitle-1]!=' ')
                    title[iTitle++] = ' ';
            }
            else
                title[iTitle++] = ch;
        }
        ch = inch_next(inch);
    }
    title[iTitle] = '\0';
 
// Delete trailing spaces from the title.
    while (iTitle>0 && title[iTitle-1]==' ')
    {   title[--iTitle] = '\0';
    }
 
// Return the character after the closing tag.
    return skipTag(inch);
}


int readHead(procHtml_t *ph)
{   inch_t *inch = ph->inch;
 
// Holds a tag.
    enum {tagMax=8};
    char tag[tagMax+1];
 
// Holds a charset.
    enum {charsetMax = 20};
    char charset[charsetMax+1];
 
// Look for a tag.
    int ch = skipTag(inch);
    while (ch != -1)
    {   if (ch == '<')
        {	inch_lookAhead(inch, tag);
            inch_next(inch);
            ch = skipWhiteSpace(inch);
            if (csc_streq(tag, "<!--"))
            {
				ch = skipComment(inch);
            }
            else if (ch == '/')
            {	inch_next(inch);
				ch = skipWhiteSpace(inch);
				if (isalpha(ch))
                {	readTagName(inch, tag, tagMax);
					ch = skipTag(inch);
					if (csc_strieq(tag, "head"))
					{   return ch;
					}
				}
            }
            else if (isalpha(ch))
            {   ch = readTagName(inch, tag, tagMax);
                if (csc_strieq(tag, "title"))
                {   ch = readTitle(inch, ph->title, procHtml_titleMax);
                }
                else if (csc_strieq(tag, "meta"))
                {   ch = readMeta(inch, charset, charsetMax);
                    if (strcmp(charset,""))
                    {   set_charset(ph, charset, csc_TRUE);
                    }
                }
                else if (csc_strieq(tag,"script") || csc_strieq(tag,"style"))
                { 
					ch = eatJscript(inch, tag);
                }
                else
                {   ch = skipTag(inch);
                }
            }
            else
            {   ch = skipTag(inch);
            }
        }
        else
        {   ch = inch_next(inch);
        }
    }
    return ch;
}


int readBody(procHtml_t *ph)
{   inch_t *inch = ph->inch;
    int nAside = 0;
 
// Holds a URL, and its associated text.
	enum {urlMax=120, urlTxtMax=200};
	char url[urlMax+1];
	char urlTxt[urlTxtMax+1];
 
// Holds a tag.
    enum {tagMax=12};
    char tag[tagMax+1];
 
// Look for a tag.
    int ch = skipTag(inch);
    while (ch != -1)
	{	if (ch == '<')
        {   bodyCh_ch(ph->bc, ' ');
			inch_lookAhead(inch, tag);
            inch_next(inch);
            ch = skipWhiteSpace(inch);
            if (csc_streq(tag, "<!--"))
            {   ch = skipComment(inch);
            }
            else if (ch == '/')
            {   inch_next(inch);
                readTagName(inch, tag, tagMax);
                ch = skipTag(inch);
                if (csc_strieq(tag, "aside"))
				{
csc_CKCK; fprintf(stderr, "readBody() nAside=%d\n", nAside);
					nAside--;
csc_CKCK; fprintf(stderr, "readBody() nAside=%d\n", nAside);
				}
                else if (csc_strieq(tag, "body"))
                {   return ch;
                }
            }
            else if (isalpha(ch))
            {   ch = readTagName(inch, tag, tagMax);
                if (csc_strieq(tag,"script") || csc_strieq(tag,"style"))
                {   ch = eatJscript(inch, tag);
					bodyCh_ch(ph->bc, ' ');
                }
				else if (csc_strieq(tag, "a"))
				{	ch = htmlTagA(ph, url, urlMax, urlTxt, urlTxtMax);
					bodyCh_ch(ph->bc, ' ');
					if (strcmp(url,""))
					{	url_t *u = url_new(url, urlTxt); 	
						if (!csc_hash_addex(ph->urls, u))
						{	url_free(u);
						}
					}
				}
				else if (csc_strieq(tag,"aside"))
				{
csc_CKCK; fprintf(stderr, "readBody() nAside=%d\n", nAside);
					nAside++;
csc_CKCK; fprintf(stderr, "readBody() nAside=%d\n", nAside);
				}
                else
                {   ch = skipTag(inch);
                }
            }
            else
            {   ch = skipTag(inch);
            }
        }
        else
        {   if (nAside < 1)
			{	bodyCh_ch(ph->bc, ch);
			}
            ch = inch_next(inch);
        }
    }
 
	return ch;
}


void procHtml_readHtml(procHtml_t *ph)
{   inch_t *inch = ph->inch;
 
// Holds a docType.
    enum {doctypeMax=5};
    char doctype[doctypeMax+1];
 
// Holds a tag.
    enum {tagMax=12};
    char tag[tagMax+1];
 
// Look for a tag.
    int ch = inch_curCh(inch);
    while (ch != -1)
    {   if (ch == '<')
        {   inch_lookAhead(inch, tag);
            inch_next(inch);
            ch = skipWhiteSpace(inch);
            if (csc_streq(tag, "<!--"))
            {
				ch = skipComment(inch);
            }
            else if (ch == '!')
            {
				ch = readSpecialTag(inch, doctype, doctypeMax);
                if (csc_strieq(doctype,"html"))
                {   if (  csc_streq(ph->explicitCharset,"")
                            && csc_streq(ph->defaultCharset,"")
                       )
                    {   set_charset(ph, "UTF-8", csc_FALSE);
                    }
                }
            }
            else if (isalpha(ch))
            {
				ch = readTagName(inch, tag, tagMax);
                if (csc_strieq(tag, "head"))
                {
					ch = readHead(ph);
                }
                else if (csc_strieq(tag, "body"))
                {
				// Set the character set.
					if (strcmp(ph->explicitCharset,""))
					{
						bodyCh_setTrans(ph->bc, ph->explicitCharset);
					}
					else if (strcmp(ph->defaultCharset,""))
					{
						bodyCh_setTrans(ph->bc, ph->defaultCharset);
					}
					else
                    {
						set_charset(ph, "ISO-8859-1", csc_FALSE);
						bodyCh_setTrans(ph->bc, ph->defaultCharset);
                    }
				// Process the body.
                    ch = readBody(ph);
                }
                else if (csc_strieq(tag,"script") || csc_strieq(tag,"style"))
                {   ch = eatJscript(inch, tag);
                }
                else
                {   ch = skipTag(inch);
                }
            }
            else
            {   ch = skipTag(inch);
            }
        }
        else
        {   ch = inch_next(inch);
        }
    }
}


void main(int argc, char **argv)
{
// Resources.
    inch_t *inch = NULL;
    procHtml_t *ph = NULL;
 
// Set up the processor.
    inch = inch_new(stdin);
 
    ph = procHtml_new( inch
                       , "http:/informationclearinghouse.info/"
                       , "index.html"
                       , ""
                     );
 
// Do the processing.
    procHtml_readHtml(ph);
 
// Debugging.
    fprintf(stderr, "title: \"%s\"\n", ph->title);
    fprintf(stderr, "explicitCharset: \"%s\"\n", ph->explicitCharset);
    fprintf(stderr, "defaultCharset: \"%s\"\n", ph->defaultCharset);
    fprintf(stderr, "baseUrl: \"%s\"\n", ph->baseUrl);
    fprintf(stderr, "thisUrl: \"%s\"\n", ph->thisUrl);
	csc_hash_iter_t *itUrls = csc_hash_iter_new(ph->urls);
	url_t *url;
	while ((url=csc_hash_iter_next(itUrls)) != NULL)
	{	fprintf(stderr, "url: \"%s\"\n", url->url);
		if (url->txt != NULL)
		{	fprintf(stderr, "urlTxt: \"%s\"\n", url->txt);
		}
	}
	csc_hash_iter_free(itUrls);
 	
// Free resources.
    if (ph != NULL)
        procHtml_free(ph);
    if (inch != NULL)
        inch_free(inch);
 
// Bye.
    exit(0);
}

