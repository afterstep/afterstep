/*
 * Copyright (c) 1998 Sasha Vasko <sasha at aftercode.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*#define DO_CLOCKING      */

#define TRUE 1
#define FALSE

#include "../configure.h"

#include "../include/asapp.h"
#include "../include/afterstep.h"
#include "../include/parser.h"

#ifdef DEBUG_PARSER
#define LOG1(a)       fprintf( stderr, a );
#define LOG2(a,b)    fprintf( stderr, a, b );
#define LOG3(a,b,c)    fprintf( stderr, a, b, c );
#define LOG4(a,b,c,d)    fprintf( stderr, a, b, c, d );
#else
#define LOG1(a)
#define LOG2(a,b)
#define LOG3(a,b,c)
#define LOG4(a,b,c,d)
#endif

HashValue
GetHashValue (char *text, int hash_size)
{
	return option_hash_value( (ASHashableValue)text, hash_size );
}

void
InitHash (SyntaxDef * syntax)
{
	register int  i;

	if (syntax->term_hash_size <= 0)
		syntax->term_hash_size = TERM_HASH_SIZE;
	if (syntax->term_hash == NULL)
		syntax->term_hash = (TermDef **) safemalloc (sizeof (TermDef *) * (syntax->term_hash_size));

	for (i = 0; i < syntax->term_hash_size; i++)
		syntax->term_hash[i] = NULL;
}

void
BuildHash (SyntaxDef * syntax)
{
	HashValue     hash_value;
	TermDef      *current;
	int           i;

	for (i = 0; syntax->terms[i].keyword; i++)
		syntax->terms[i].brother = NULL;

	for (i = 0; syntax->terms[i].keyword; i++)
	{
		hash_value = option_hash_value( (ASHashableValue)(syntax->terms[i].keyword), syntax->term_hash_size );
		if (syntax->term_hash[hash_value])
		{
			for (current = syntax->term_hash[hash_value]; current->brother; current = current->brother);
			current->brother = &(syntax->terms[i]);
		} else
			syntax->term_hash[hash_value] = &(syntax->terms[i]);
	}
}

TermDef      *
FindTerm (SyntaxDef * syntax, int type, int id)
{
	register int  i;

	if (syntax)
		for (i = 0; syntax->terms[i].keyword; i++)
			if (type == TT_ANY || type == syntax->terms[i].type)
				if (id == ID_ANY || id == syntax->terms[i].id)
					return &(syntax->terms[i]);

	return NULL;
}

/* this one will identify Term using Hash table
   it will seT :
   - current_term to the term found
   Returns : NULL if no terms found, otherwise same as current_term.
 */
TermDef      *
FindStatementTerm (char *tline, SyntaxDef * syntax)
{
	HashValue     hash_value;
	TermDef      *pterm;

	hash_value = option_hash_value( AS_HASHABLE(tline), syntax->term_hash_size);
	for (pterm = syntax->term_hash[hash_value]; pterm; pterm = pterm->brother)
		if (option_compare (AS_HASHABLE(tline), AS_HASHABLE(pterm->keyword)) == 0)
			return pterm;
	return NULL;
}
