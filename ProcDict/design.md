								  +=======================+
								  #	Processing Dictionary #
								  +=======================+


Abstract
========

Makes a custom dictionary from the standard Linux dictionaries in the form a plain text file.  

Each line (each entry) consists of a lookup word possibly followed by
its substitution word, if the lookup word is to be substituted. 


Status
======
*	Complete for now.  Code seems to work OK.  Produces useful dictionary.


About this document
===================
This document is about the creation of a special dictionary that maps
some words to others.  It covers 4 issues:
1) Plurals
2) Possessives
3) Contractions
4) Americanisation
5) Repeated consonants.


It does not cover the contraction of double letters into single letters.


Motivation
==========

I want:
*	I want canine, canines and canine's to all match.
*	I want minimized, minimize, minimise, minimising and minimised to all match.

The obvious way to achieve that is to convert them to canonical form
(root word using American spelling) both when the data is collected and
during a search.


Intended approach
=================

Runtime Algorithm
-----------------
*	Word converted to lowercase.
*	Double consonants removed.
	*	batting becomes bating.
*	Apply lookup table.
	*	don't -> dont (don't becomes dont)
	*	dog's -> dog
	*	dogs -> dog
	*	dictionaries -> dictionary
	*	visualisation -> visualize

Lookup Table
------------
The lookup table is the dictionary.  It may have other uses too.
It will be read from a file.  The file format consists of words, one
lookup per
line.  If a line has a second word, then the lookup word is the first
word in a word string, and the second is a substitution word or entry.
When the table is complete, the second word will be the root word, but
while it is being developed, the second word may point at a word that if
we follow the chain will eventually lead us to the root word.

One advantage of having the plain text file form is that it can be
edited.  I expect using general rules will only go so far, and
exceptions can be manually edited.

I am going to use the american and british dictionaries provided with
Linux (/usr/share/dict/) as my starting point.


Creating the lookup table
=========================

Americanisation
---------------

1)
*	Create table "Diff1" of words different between American and British.
*	Create a dictionary table "Dict" from American English.
*	Create a table "Brit" of British English.

*	For each American word in Diff1
	*	If replacing a "ize$" in word X with an "ise" makes a word Y in Brit
			that does not exist in Dict
		*	add a substitution entry Y -> X to X in Dict. 
	*	Else if replacing a "er$" in word X with an "re" makes a word Y
			in Brit that does not exist in Dict
		*	add a substitution Y to entry X in Dict. 
	*	Else if replacing a "ai" in word X with an "ae" makes a word Y in Brit
			that does not exist in Dict
		*	add a substitution Y to entry X in Dict. 
	*	Else if replacing a "ne" in word X with an "gn" makes a word Y in Brit
			that does not exist in Dict
		*	add a substitution entry Y -> X to X in Dict. 
	*	Else if replacing a "ov" in word X with an "oov" makes a word Y in Brit
			that does not exist in Dict
		*	add a substitution Y to entry X in Dict. 
	*	Else if replacing a "to" in word X with an "tte" makes a word Y in Brit
			that does not exist in Dict
		*	add a substitution Y to entry X in Dict. 
	*	Else if replacing a "a" in word X with an "ae" makes a word Y in Brit
			that does not exist in Dict
		*	add a substitution Y to entry X in Dict. 
	*	Else if replacing a "o" in word X with an "ou" makes a word Y in Brit
			that does not exist in Dict
		*	add a substitution Y to entry X in Dict. 
	*	Else if replacing a "e" in word X with an "oe" makes a word Y in Brit
			that does not exist in Dict
		*	add a substitution Y to entry X in Dict. 
	*	Else if replacing a "l" in word X with an "ll" makes a word Y in Brit
			that does not exist in Dict
		*	add a substitution Y to entry X in Dict. 
	*	Else if the removal of a letter "e" in word X makes a word Y in Brit
			that does not exist in Dict
		*	add a substitution Y to entry X in Dict. 

*	For all words in Dict that do not have a substitution:
	*	If replacing a "tion$" in word X with an "te" makes a word Y in Dict
		*	add a substitution entry X -> Y to X in Dict. 
	*	else if removing "ies$" in word X makes a word Y  ($ matches end of word).
		*	add a substitution entry X -> Y to X in Dict. 
	*	else if removing "ing$" in word X makes a word Y  ($ matches end of word).
		*	add a substitution entry X -> Y to X in Dict. 
	*	else if removing "ed$" in word X makes a word Y
		*	add a substitution entry X -> Y to X in Dict. 
	*	else if removing "es$" in word X makes a word Y
		*	add a substitution entry X -> Y to X in Dict. 
	*	else if removing "s$" in word X makes a word Y
		*	add a substitution entry X -> Y to X in Dict. 

*	For all words X in Dict that have a substitution Y.
	*	F = Y
	*	while (F has a substitution)
		*	F = subst(F)
	*	If F != Y
		*	Replace X->Y with X->F

*	There are 60,000 words there.  I probably cant find all the wrong
	substitutions, but many of the errors can be found at
	the end, (unrolling the substitutions).  Remove found wrong
	substitutions.  Make a list.  Then pull out the wrong substitions
	BEFORE the unrolling step.


2)
*	Apply substitutions to Brit.
*	Sort Brit.
*	Make new Diff2
*	Observe and see what to do.


Plurals and Possessives
-----------------------

*	In the following algorithm, "Otherwise" means
	"if no substitution yet for the word".

*	For each word in Dict: 
	*	If a (word X ends in "ies")  
		*	If substituting "ies" with "y" results in word Y.
			*	add substitution Y to entry X.

	*	"Otherwise" if a (word X ends in "'s")  
		*	If removing "'s" from X results in word Y.
			*	add substitution Y to entry X.

	*	"Otherwise" if a (word X ends in "es")  
		*	If removing "es" from X results in word Y.
			*	add substitution Y to entry X.

	*	"Otherwise" if a (word X ends in "s")  
		*	If removing "s" from X results in word Y.
			*	add substitution Y to entry X.

Issues
-------

What you have
*	demilitarization's demilitarization
*	demilitarisation's demilitarization's
*	demilitarisation demilitarization
*	demilitarization

What you want
*	demilitarisation   demilitarization
*	demilitarisation's demilitarization
*	demilitarization's demilitarization
*	demilitarization
