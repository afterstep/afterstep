/*
 * Copyright (c) 2000 Andrew Ferguson <andrew@owsla.cjb.net>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#undef DO_CLOCKING
#undef UNKNOWN_KEYWORD_WARNING
#define LOCAL_DEBUG

#include "../configure.h"

#include <fcntl.h>
#include <unistd.h>

#ifdef DO_CLOCKING
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#endif
#include "asapp.h"
#include "afterstep.h"
#include "parser.h"
#include "screen.h"
#include "freestor.h"
#include "functions.h"
#include "../libAfterImage/asimagexml.h"

extern char  *_disabled_keyword;
#if 0
static char  *_as_nothing_ = "";
#endif

struct
{
	char *label;
	int value;
}FixedEnumValues[] =
{
#define FEV

	{NULL, 0}
};

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


/* debugging stuff */
void
freestorage_print(char *myname, SyntaxDef *syntax, FreeStorageElem * storage, int level)
{
	if( myname == NULL ) 
		myname = MyName;
	while (storage)
	{
		char sub_terminator = ' ' ;
		if( level > 0 ) 
			if( syntax->terminator == '\n' || syntax->terminator == '\0' ) 
				fprintf (stderr, "%*s", level*4, " ");

		if( !get_flags( storage->term->flags, TF_NO_MYNAME_PREPENDING )  )
		{	
			fprintf (stderr, "*%s", myname);
			if( storage->term->keyword[0] == '~' )
				fputc( ' ', stderr );
		}
		fprintf (stderr, "%s", storage->term->keyword);
		{
			register int  i;
			int token_count = storage->argc ; 
			if( storage->term->sub_syntax ) 
			{
				token_count = GetTermUseTokensCount(storage->term);
				if( get_flags( storage->term->flags, TF_NAMED|TF_INDEXED|TF_DIRECTION_INDEXED) )
					++token_count;
			}	 

			for (i = 0; i < token_count; i++)
				fprintf (stderr, " %s", storage->argv[i]);
			if( token_count == 0 && storage->sub && storage->term->sub_syntax )
				fputc( ' ', stderr );
		}
		if( storage->sub && storage->term->sub_syntax ) 
		{	
			if( storage->term->sub_syntax->terminator == '\n' ) 
				fputc( '\n', stderr );		
 			freestorage_print(myname, storage->term->sub_syntax, storage->sub, level+1);	
			sub_terminator = storage->term->sub_syntax->file_terminator ;
			if( sub_terminator == '\0' ) 
				sub_terminator = '\n' ;
		}
		if( storage->next &&  syntax->terminator != '\0' && (sub_terminator == ' ' || sub_terminator != syntax->terminator ) ) 
			fputc( syntax->terminator, stderr );
		storage = storage->next;
	}
	if( syntax->file_terminator != '\0' ) 
		fputc( syntax->file_terminator, stderr );
	else
		fputc( '\n', stderr );
}

/* StringArray functionality */
char        **
CreateStringArray (size_t elem_num)
{
	char        **array = (char **)safemalloc (elem_num * sizeof (char *));
	int           i;

	for (i = 0; i < elem_num; i++)
		array[i] = NULL;
	return array;
}

size_t GetStringArraySize (int argc, char **argv)
{
	size_t        size = 0;

	for (argc--; argc >= 0; argc--)
		if (argv[argc])
			size += strlen (argv[argc]) + 1;
	return size;
}

char         *
CompressStringArray (int argc, char **argv)
{
	size_t        size = 0;
	register char *end = argv[argc - 1], *cur = argv[0], *cur2;
	char         *dst;

	while (*end)
		end++;
	size = end - cur + 1;

	cur2 = dst = (char *)safemalloc (size);

	for (; cur < end; cur++, cur2++)
		if (*cur)
			*cur2 = *cur;
		else
			*cur2 = ' ';

	*cur2 = '\0';

	return dst;
}

char        **
DupStringArray (int argc, char **argv)
{
	int           i;
	size_t        data_size;
	char        **array = CreateStringArray (argc);

	data_size = GetStringArraySize (argc, argv);
	if (data_size && array)
	{
		array[0] = (char *)safemalloc (data_size);
		for (i = 0; i < data_size; i++)
			array[0][i] = argv[0][i];
		for (i = 1; i < argc; i++)
			if (argv[i])
				array[i] = array[0] + (argv[i] - argv[0]);
	}
	return array;
}

void
AddStringToArray (int *argc, char ***argv, char *new_string)
{
	int           i = 0;
	size_t        data_size;
	char        **array;

	if (new_string == NULL)
		return;

	array = CreateStringArray (*argc + 1);
	if (array)
	{
		data_size = GetStringArraySize (*argc, *argv);
		array[0] = (char *)safemalloc (data_size + strlen (new_string) + 1);
		if (data_size > 0)
		{
			memcpy (array[0], (*argv)[0], data_size);
			for (i = 1; i < *argc; i++)
				if ((*argv)[i])
					array[i] = array[0] + ((*argv)[i] - (*argv)[0]);
			array[i] = array[0] + data_size;
			free (*argv[0]);
		}
		if (*argv)
			free (*argv);
		strcpy (array[i], new_string);
		(*argc)++;
		*argv = array;
	}
}

/* end StringArray functionality */


/*********************************************************************************************/
/*                                     FreeStorage management                                */
/*********************************************************************************************/
/* Create new FreeStorage Elem and add it to the supplied storage's tail */
FreeStorageElem *
AddFreeStorageElem (SyntaxDef * syntax, FreeStorageElem ** tail, TermDef * pterm, int id)
{
	FreeStorageElem *new_elem = NULL;

	if (pterm == NULL)
		pterm = FindTerm (syntax, TT_ANY, id);

	if (pterm)
	{
		new_elem = (FreeStorageElem *) safecalloc (1, sizeof (FreeStorageElem));
		new_elem->term = pterm;
		new_elem->next = *tail;
		*tail = new_elem;
	}
	return new_elem;
}

/* Duplicate existing FreeStorage Elem */
FreeStorageElem *
DupFreeStorageElem (FreeStorageElem * source)
{
	FreeStorageElem *new_elem = NULL;

	if (source)
	{
		new_elem = (FreeStorageElem *) safecalloc (1, sizeof (FreeStorageElem));
		new_elem->term = source->term;
		new_elem->argc = source->argc;
		/* duplicating argv here */
		new_elem->argv = DupStringArray (source->argc, source->argv);
		new_elem->flags = source->flags;
		if (new_elem->sub)
		{
			FreeStorageElem *psub, **pnew_sub = &(new_elem->sub);

			for (psub = new_elem->sub; psub; psub = psub->next)
			{
				*pnew_sub = DupFreeStorageElem (psub);
				pnew_sub = &((*pnew_sub)->next);
			}
		}
	}
	return new_elem;
}

void
ReverseFreeStorageOrder (FreeStorageElem ** storage)
{
	FreeStorageElem *pNewHead = NULL, *pNext;

	for (; *storage; *storage = pNext)
	{
		pNext = (*storage)->next;
		(*storage)->next = pNewHead;
		pNewHead = *storage;
	}
	*storage = pNewHead;
}

void
CopyFreeStorage (FreeStorageElem ** to, FreeStorageElem * from)
{
	FreeStorageElem *pNew;

	if (to == NULL)
		return;
	for (; from; from = from->next)
	{
		if ((pNew = DupFreeStorageElem (from)) == NULL)
			continue;
		pNew->next = *to;
		*to = pNew;
	}
}

int
CountFreeStorageElems (FreeStorageElem * storage)
{
	int           count = 0;

	for (; storage; storage = storage->next)
		count++;
	return count;
}

/* this one will scan list of FreeStorage elements and will move all elements with
   specifyed flags mask into the garbadge_bin
 */

void
StorageCleanUp (FreeStorageElem ** storage, FreeStorageElem ** garbadge_bin, unsigned long mask)
{
	FreeStorageElem **ppCurr, *pToRem;

	for (ppCurr = storage; *ppCurr; ppCurr = &((*ppCurr)->next))
	{
		while((*ppCurr)->flags & mask)
		{
			pToRem = *ppCurr;
			*ppCurr = pToRem->next;
			pToRem->next = *garbadge_bin;
			*garbadge_bin = pToRem;
			if( *ppCurr == NULL )
				return;
		}
		if ( *ppCurr && (*ppCurr)->sub)
			StorageCleanUp (&((*ppCurr)->sub), garbadge_bin, mask);
	}
}

/* memory deallocation */
void
DestroyFreeStorage (FreeStorageElem ** storage)
{
	if (storage)
		if (*storage)
		{
			DestroyFreeStorage (&((*storage)->next));
			DestroyFreeStorage (&((*storage)->sub));
			/* that will deallocate everything */
			if ((*storage)->argc && (*storage)->argv)
			{
				if ((*storage)->argv[0])
#ifdef DEBUG_PARSER
				{
					fprintf (stderr, "\n DestroyFreeStorage: deallocating [%s].", (*storage)->argv[0]);
#endif
					free ((*storage)->argv[0]);
#ifdef DEBUG_PARSER
				} else
					fprintf (stderr, "\n DestroyFreeStorage: no data to deallocate.");
#endif
				free ((*storage)->argv);
			}
			free (*storage);
			*storage = NULL;
		}
}

/* create and format freestorage elem from the statement found */
void
args2FreeStorage (FreeStorageElem * pelem, char *data, int data_len)
{
	int           argc = 0, max_argc = 0;
	char        **argv = NULL;

	if (data_len > 0)
	{
		register char *cur, *dst;

		if (!(pelem->term->flags & TF_DONT_SPLIT))
		{
			cur = data;
			while (*cur)
			{
				if (!isspace ((int)*cur))
					max_argc++;
				while (!isspace ((int)*cur) && *cur)
					cur++;
				while (isspace ((int)*cur) && *cur)
					cur++;
			}
		} else
		{
			max_argc++;
            if (get_flags(pelem->term->flags, (TF_INDEXED|TF_DIRECTION_INDEXED)))
				max_argc++;
		}

		if (max_argc > 0)
		{
			argv = CreateStringArray (max_argc);
			dst = argv[0] = (char *)safemalloc (data_len + 1);
			cur = data;
			max_argc--;
			while (argc < max_argc)
			{
				dst = argv[argc];
				while (!isspace ((int)*cur) && *cur)
				{
					if (*cur == '"')
					{
						register char *ptr = find_doublequotes (cur);

						if (ptr)
							while (cur != ptr)
								*(dst++) = *(cur++);
					}
					*(dst++) = *(cur++);
				}
				if (dst <= argv[argc])
					break;

				*(dst++) = '\0';
				if (++argc <= max_argc)
					argv[argc] = dst;

				while (isspace ((int)*cur) && *cur)
					cur++;
			}
			if (*cur && argc <= max_argc)
				strcpy (argv[argc++], cur);
		}
	}
	pelem->argc = argc;
	pelem->argv = argv;
}

/****************************************************************************/
/* F2D: FreeStorage to Datatype conversion routines :			    */
/****************************************************************************/
/* free storage post processing code */
FunctionData *
free_storage2func (FreeStorageElem * stored, int *ppos)
{
	register struct FunctionData *pfunc;
	int           pos = *ppos;

	if (stored->term == NULL)
		return NULL;

	pfunc = (struct FunctionData *)safecalloc (1, sizeof (struct FunctionData));

	pfunc->func = stored->term->id;

	/* setting up reasonable defaults */
    set_func_val (pfunc, -1, default_func_val(pfunc->func));
LOCAL_DEBUG_OUT("argv=%p, arc=%d, sub=%p", stored->argv, stored->argc, stored->sub );
	if (stored->argv == NULL)
		return pfunc;

	/* first checking if we have doublequoted name */
	if (pos < stored->argc && *(stored->argv[pos]) == '"')
	{
		register int  len = strlen (stored->argv[pos]);

		if (len > 2)
		{									   /* we want to strip off those doubleqoutes */
			pfunc->name = mystrndup (stored->argv[pos] + 1, len - 2);
            pfunc->hotkey = scan_for_hotkey (pfunc->name);
		}
        /* otherwise - empty name, but we should still skip it */
        ++pos ;
	}
	/* now storing the rest of parameters as text */
	if (pos < stored->argc && stored->argv[pos])
	{
		pfunc->text = CompressStringArray (stored->argc - pos, &(stored->argv[pos]));
	}
	while (pos < stored->argc && stored->argv[pos])
	{
		register char *cur = stored->argv[pos];

		if (isdigit ((int)*cur) || ((*cur == '-' || *cur == '+') && isdigit ((int)*(cur + 1))))
			break;
		pos++;
	}

	if (pos < stored->argc && stored->argv[pos])
	{
		parse_func_args (stored->argv[pos], &(pfunc->unit[0]), (int *)&(pfunc->func_val[0]));
		pos++;
	}
	if (pos < stored->argc && stored->argv[pos])
		parse_func_args (stored->argv[pos], &(pfunc->unit[1]), (int *)&(pfunc->func_val[1]));

    decode_func_units (pfunc);

	*ppos = pos;
	return pfunc;
}

/* END of FunctionData - related code                                        */

int          *
free_storage2int_array (FreeStorageElem * stored, int *ppos)
{
	int           pos = *ppos;
	int           count = 0;
	int          *array = NULL;

	if (stored)
	{
		if (pos < stored->argc)
		{
			array = safemalloc (sizeof (int) * (stored->argc - pos));

			do
			{
				array[count++] = atoi (stored->argv[pos]);
			}
			while (++pos < stored->argc);
		}
		*ppos = pos;
	}
	return array;
}

char         *
free_storage2quoted_text (FreeStorageElem * stored, int *ppos)
{
	char         *ptr = stored->argv[*ppos], *tail;
	char         *result = NULL;

	if (*ptr != '"')
		ptr = find_doublequotes (ptr);
	if( ptr == NULL && get_flags( stored->term->flags, TF_QUOTES_OPTIONAL) )
	{
	 	result = mystrdup(stored->argv[*ppos]);		
		(*ppos)++;
	}else if (ptr)
	{
		ptr++;
		if ((tail = find_doublequotes (ptr)) == NULL)
		{
			show_error("terminating quote missing in [%s]! Using whole line !", stored->argv[*ppos]);
			result = mystrdup (ptr);
		} else
			result = mystrndup (ptr, tail - ptr);
		/* stripping those backslashed doubleqoutes off of the text */
		for (ptr = result + 1; *ptr; ptr++)
			if (*ptr == '"' && *(ptr - 1) == '\\')
				strcpy (ptr - 1, ptr);
		(*ppos)++;
	} else
		show_error ("bad quoted string [%s] (ppos == %d) for option [%s]. Ignoring!",ptr?ptr:"(null)", *ppos, stored->term->keyword);

	return result;
}

Bool
free_storage2box (FreeStorageElem * stored, int *ppos, ASBox * pbox)
{
	register int  i = 0;
	int          *data[4];
	int           flags_list[4] = { LeftValue, TopValue, RightValue, BottomValue };
	int           sign_flags_list[4] = { LeftNegative, TopNegative, RightNegative, BottomNegative };

	if (pbox != NULL && ppos != NULL && stored != NULL)
	{
		data[0] = &(pbox->left);
		data[1] = &(pbox->top);
		data[2] = &(pbox->right);
		data[3] = &(pbox->bottom);
		pbox->flags = 0;
		for (; *ppos < stored->argc && i < 4; (*ppos)++, i++)
		{
			*(data[i]) = atoi (stored->argv[*ppos]);
			pbox->flags |= flags_list[i];
			if (*(data[i]) < 0 || *(stored->argv[*ppos]) == '-')
				pbox->flags |= sign_flags_list[i];
		}
	}
	return (i == 0) ? False : True;
}

Bool
free_storage2geometry (FreeStorageElem * stored, int *ppos, ASGeometry * pgeometry)
{
	char *geom = stored->argv[(*ppos)++];

	if (pgeometry == NULL || geom == NULL )
		return False;
	memset( pgeometry, 0x00, sizeof(ASGeometry)) ;
	parse_geometry ( geom,
	                &(pgeometry->x), &(pgeometry->y),
					&(pgeometry->width), &(pgeometry->height),
					&(pgeometry->flags));
	return True;
}

ASButton     *
free_storage2button (FreeStorageElem * stored, int *ppos)
{
    register ASButton *pbutton = NULL;
    register int s ;
    pbutton = safecalloc( 1, sizeof(ASButton));
	for( s = 0 ; s < ASB_StateCount && *ppos < stored->argc ; ++s )
	{
        pbutton->shapes[s] = mystrdup (stored->argv[(*ppos)++]);
	}
    return pbutton;
}


typedef struct charkey_xref
{                                              /* unused ???? */
  char key;
  int value;
}charkey_xref;


/* The keys must be in lower case! */
#ifdef C_TITLEBAR
static charkey_xref as_contexts[] = {
    {'1', C_TButton1},
    {'2', C_TButton2},
    {'3', C_TButton3},
    {'4', C_TButton4},
    {'5', C_TButton5},
    {'6', C_TButton6},
    {'7', C_TButton7},
    {'8', C_TButton8},
    {'9', C_TButton9},
    {'0', C_TButton0},
    {'B', C_TITLEBAR},
    {'C', C_FRAME_N},
    {'D', C_FRAME_E},
    {'E', C_FRAME_W},
    {'G', C_FRAME_NW},
    {'H', C_FRAME_NE},
    {'J', C_FRAME_SW},
    {'I', C_ICON},
    {'K', C_FRAME_SE},
    {'N', C_NAME },
    {'O', C_ICON_NAME },
    {'P', C_CLASS_NAME },
    {'Q', C_RES_NAME },
    {'R', C_ROOT},
    {'S', C_FRAME_S},
    {'W', C_WINDOW},
    {'F', C_FRAME},
    {'T', C_TITLE},
    {'A', C_ALL},
	{0, 0}
};
#else
struct charkey_xref as_contexts[] = {
    {'W', C_WINDOW},
    {'T', C_TITLE},
    {'I', C_ICON},
    {'R', C_ROOT},
    {'F', C_FRAME},
    {'S', C_SIDEBAR},
    {'1', C_TButton1},
    {'2', C_TButton2},
    {'3', C_TButton3},
    {'4', C_TButton4},
    {'5', C_TButton5},
    {'6', C_TButton6},
    {'7', C_TButton7},
    {'8', C_TButton8},
    {'9', C_TButton9},
    {'0', C_TButton0},
    {'A', C_WINDOW | C_TITLE | C_ICON | C_ROOT | C_FRAME | C_SIDEBAR | C_TButtonAll},
	{0, 0}
};

#endif

/* The keys musat be in lower case! */
static charkey_xref key_modifiers[] = {
	{'S', ShiftMask},
	{'C', ControlMask},
	{'M', Mod1Mask},
	{'1', Mod1Mask},
	{'2', Mod2Mask},
	{'3', Mod3Mask},
	{'4', Mod4Mask},
	{'5', Mod5Mask},
	{'A', AnyModifier},
	{'N', 0},
	{0, 0}
};

/*
 * Turns a  string context of context or modifier values into an array of
 * true/false values (bits)
 * returns pointer to the fisrt character after context specification
 */
static void
string2context (char *string, int *output, charkey_xref *table)
{
    register int  i, j;
	int          tmp1;

	*output = 0;
    for (i = 0 ; string[i] ; i++)
	{
		/* in some BSD implementations, tolower(c) is not defined
		 * unless isupper(c) is true */
        tmp1 = string[i];
		if (islower (tmp1))
			tmp1 = toupper (tmp1);
		/* end of ugly BSD patch */
		for (j = 0; table[j].key != 0; j++)
			if (tmp1 == table[j].key)
			{
				*output |= table[j].value;
				break;
			}
		if (table[j].key == 0)
            show_error("undefined context [%c] requested in [%s].", string[i], string);
	}
}

/*
 * Turns a  string context of context or modifier values into an array of
 * true/false values (bits)
 * returns pointer to the fisrt character after context specification
 */

char         *
parse_context (char *string, int *output, charkey_xref *table)
{
	register int  j;
	register char *ptr;
	char          tmp1;

	*output = 0;
	for (ptr = string; isspace (*ptr); ptr++);
	for (; *ptr && !isspace (*ptr); ptr++)
	{
		/* in some BSD implementations, tolower(c) is not defined
		 * unless isupper(c) is true */
		tmp1 = *ptr;
        if (islower (tmp1))
            tmp1 = toupper (tmp1);
		/* end of ugly BSD patch */
		for (j = 0; table[j].key != 0; j++)
			if (tmp1 == table[j].key)
			{
				*output |= table[j].value;
				break;
			}
		if (table[j].key == 0)
		{
            show_error("undefined context specifyer [%c] encountered in [%s].", *ptr, string);
		}
	}
	return ptr;
}

int
parse_modifier( char *tline )
{
    int mods = 0;
    parse_context (tline, &mods, key_modifiers);
    return mods;
}

int parse_win_context (char *tline)
{
    int ctxs = 0;
    parse_context (tline, &ctxs, as_contexts);
	LOCAL_DEBUG_OUT( "tline = \"%s\", ctxs = %X", tline, ctxs ); 
    return ctxs;
}

char *parse_modifier_str( char *tline, int *mods_ret )
{
    int mods = 0;
	LOCAL_DEBUG_OUT( "tline = \"%s\"", tline ); 
    tline = parse_context (tline, &mods, key_modifiers);
	LOCAL_DEBUG_OUT( "tline = \"%s\", mods = %X", tline, mods ); 
	*mods_ret = mods;
    return tline;
}

char *parse_win_context_str (char *tline, int *ctxs_ret)
{
    int ctxs = 0;
	LOCAL_DEBUG_OUT( "tline = \"%s\"", tline ); 
    tline = parse_context (tline, &ctxs, as_contexts);
    *ctxs_ret = ctxs ;
	LOCAL_DEBUG_OUT( "tline = \"%s\", ctxs = %X", tline, ctxs ); 
	return tline;
}

static void
context2string (char *string, int input, charkey_xref *table, Bool terminate)
{
    register int  i, j = 0;

    i = 0 ;
    if( input != 0 || !terminate )
		for (j = 0; table[j].key != 'A'; j++)
            if( input & table[j].value )
                string[i++] = table[j].key ;
	if( terminate )
    {
        while ( string[i] != '\0' && !isspace((int)string[i])) i++;
	    string[i] = '\0' ;
    }else
	    string[j] = '\0' ;
}

char     *
free_storage2binding (FreeStorageElem * stored, int *ppos, int *context, int *mods )
{
    char *sym = NULL ;
    if (*ppos < stored->argc)
        sym = mystrdup (stored->argv[(*ppos)++]);
    if( sym )
    {
        if (*ppos < stored->argc && context )
            string2context( stored->argv[(*ppos)++], context, as_contexts );
        if (*ppos < stored->argc && mods )
            string2context( stored->argv[(*ppos)++], mods, key_modifiers );
    }
    return sym;
}

ASCursor     *
free_storage2cursor (FreeStorageElem * stored, int *ppos)
{
    register ASCursor *pcursor = safecalloc( 1, sizeof(ASCursor));

	if (*ppos < stored->argc)
        pcursor->image_file = mystrdup (stored->argv[(*ppos)++]);
	if (*ppos < stored->argc)
        pcursor->mask_file = mystrdup (stored->argv[(*ppos)++]);
    return pcursor;
}

long
free_storage2bitlist (FreeStorageElem * stored, int *ppos )
{
    long bits = 0 ;
    int  bit_num ;
    Bool valid ;
    register int i ;
    register char *ptr ;

    if( *ppos == stored->argc )                 /* by default - everything is set */
        return 0xFFFFFFFF;

    for( i = *ppos; i < stored->argc ; i++ )
    {
        ptr = stored->argv[i] ;
        while ( *ptr )
        {
            while( *ptr && !isdigit((int)*ptr)) ptr++ ;
            bit_num = 0 ;
            valid = False ;
            while( *ptr && isdigit((int)*ptr))
            {
                valid = True ;
                bit_num = bit_num*10+((*ptr)-'0');
                ptr++ ;
            }
            if( valid && bit_num < sizeof(long)*8 )
                bits |= (0x01<<bit_num) ;
        }
    }

    return bits;
}

#ifndef ASCURSOR_H_HEADER_INCLUDED
void
destroy_ascursor (ASCursor ** cursor)
{
    if (cursor)
        if (*cursor)
		{
			register ASCursor *c = *cursor ;

            if (c->image_file)
                free (c->image_file);
            if (c->mask_file)
                free (c->mask_file);
            free (*cursor);
            *cursor = NULL;
		}
}

#endif
static void
FreeConfigItem (ConfigItem * item)
{
	switch (item->type)
	{
     case TT_INTARRAY:
         item->data.int_array.array = NULL;
		 item->data.int_array.size = 0;
     case TT_BINDING :
         item->data.binding.sym = NULL ;
     case TT_COLOR:
	 case TT_FONT:
	 case TT_FILENAME:
	 case TT_TEXT:
	 case TT_PATHNAME:
     case TT_QUOTED_TEXT:
     case TT_OPTIONAL_PATHNAME:
         item->data.string = NULL;
         if( item->memory )
            free (item->memory);
         break;

	 case TT_FUNCTION:
		 free_func_data (item->data.function);
		 free (item->data.function);
		 item->data.function = NULL;
		 break;

	 case TT_BUTTON:
#if 1
		 destroy_asbutton (item->data.button, False);
#endif
		 break;
     case TT_CURSOR:
         destroy_ascursor (&(item->data.cursor));
		 break;
    }
	item->memory = NULL;
}

static        Bool
check_avail_args (FreeStorageElem * stored, int pos, int need)
{
	if (stored->argv == NULL || stored->argc < pos + need)
		return False;
	while (--need >= 0)
		if (stored->argv[pos + need] == NULL)
			return False;
	return True;
}

int 
direction2index(const char *ptr)	
{
	int index = 0;             
    /* custom code to parse frame sides very fast :*/

	if( ptr[0] == 'W' || ptr[0] == 'w' )
        index = FR_W ;
    else if( ptr[0] == 'E' || ptr[0] == 'e' )
        index = FR_E ;
    else if( ptr[0] == 'N' || ptr[0] == 'n' )
    {
        if( ptr[1] == 'W' || ptr[1] == 'w' )
            index = FR_NW ;
        else if( ptr[1] == 'E' || ptr[1] == 'e' )
            index = FR_NE ;
        else 
        {
            register int i = 0;
            while( i < 5 && ptr[i] != '\0' ) ++i;
            if( ptr[i] == 'W' || ptr[i] == 'w' )
                index = FR_NW ;
            else if( ptr[i] == 'E' || ptr[i] == 'e' )
                index = FR_NE ;
            else
                index = FR_N ;
        }
    }else if( ptr[0] == 'S' || ptr[0] == 's' )
    {
        if( ptr[1] == 'W' || ptr[1] == 'w' )
            index = FR_SW ;
        else if( ptr[1] == 'E' || ptr[1] == 'e' )
            index = FR_SE ;
        else
        {
            register int i = 0;
            while( i < 5 && ptr[i] != '\0' ) ++i;
            if( ptr[i] == 'W' || ptr[i] == 'w' )
                index = FR_SW ;
            else if( ptr[i] == 'E' || ptr[i] == 'e' )
                index = FR_SE ;
            else
                index = FR_S ;
        }
    }
	return index;
}

int
ReadConfigItem (ConfigItem * item, FreeStorageElem * stored)
{
	if (item->memory && item->ok_to_free)
		FreeConfigItem (item);

	if (stored)
	{
		int           pos = 0;
		int           ret_code = 0;

		item->memory = NULL;
		item->ok_to_free = 0;
		item->type = stored->term->type;
		if (item->type == TT_FLAG)
		{
			item->data.flag = True;
			ret_code = 1;
		}

        if (get_flags(stored->term->flags, TF_INDEXED))
		{
			if (!check_avail_args (stored, pos, 1))
				return ret_code;
			item->index = atoi (stored->argv[pos++]);
        }else if (get_flags(stored->term->flags, TF_DIRECTION_INDEXED))
        {
			if (!check_avail_args (stored, pos, 1))
				return ret_code;
            item->index = direction2index(stored->argv[pos++]);
        }
		switch (item->type)
		{
		 case TT_FLAG:
			 if (check_avail_args (stored, pos, 1))
				 if (atol (stored->argv[pos++]) <= 0)
					 item->data.flag = False;
			 break;
		 case TT_INTEGER:
		 case TT_UINTEGER:
			 if (!check_avail_args (stored, pos, 1))
				 return 0;
#if 0
			 if( !isdigit(stored->argv[pos][0]) && stored->argv[pos][0] != '-' )
			 {


			 }else
			 	item->data.integer = atol (stored->argv[pos]);
#endif
			 item->data.integer = parse_math(stored->argv[pos], NULL, 0);
			 ++pos ;
			 if (stored->term->type == TT_UINTEGER && item->data.integer < 0)
			 {
				 show_warning("negative value %ld assigned to %s, that accepts only positive numbers! Ignoring!\n",
						    item->data.integer, stored->term->keyword);
				 return 0;
			 }
			 break;
	         case TT_BITLIST :
           		  item->data.integer = free_storage2bitlist(stored,&pos);
		          break;
         case TT_OPTIONAL_PATHNAME:
             if (!check_avail_args (stored, pos, 1))
             {
                item->data.string = NULL;
                return 1;
             }
         case TT_COLOR:
		 case TT_FONT:
		 case TT_FILENAME:
		 case TT_TEXT:
		 case TT_PATHNAME:
			 if (!check_avail_args (stored, pos, 1))
				 return 0;
			 if (stored->term->type != TT_TEXT && *(stored->argv[pos]) == '"')
			 {
				 item->memory = item->data.string = stripcpy2 (stored->argv[pos], 0);
			 } else
			 {
				 item->memory = safemalloc (strlen (stored->argv[pos]) + 1);
				 item->data.string = (char *)(item->memory);
                 strcpy (item->data.string, stored->argv[pos]);
			 }
             ++pos;
			 break;
		 case TT_QUOTED_TEXT:
			 if (!check_avail_args (stored, pos, 1))
				 return 0;
			 item->data.string = free_storage2quoted_text (stored, &pos);
			 item->memory = (void *)item->data.string;
			 break;
		 case TT_BOX:
			 if (!check_avail_args (stored, pos, 1))
				 return 0;
			 if (!free_storage2box (stored, &pos, &(item->data.box)))
				 return 0;
			 break;
		 case TT_GEOMETRY:
			 if (!check_avail_args (stored, pos, 1))
				 return 0;
			 if (!free_storage2geometry (stored, &pos, &(item->data.geometry)))
				 return 0;
			 break;
		 case TT_FUNCTION:
			 item->data.function = free_storage2func (stored, &pos);
			 item->memory = (void *)item->data.function;
			 break;
		 case TT_BUTTON:
			 item->data.button = free_storage2button (stored, &pos);
			 item->memory = (void *)item->data.button;
			 break;
		 case TT_INTARRAY:
			 {
				 item->data.int_array.size = pos;
				 item->data.int_array.array = free_storage2int_array (stored, &pos);
				 item->data.int_array.size = pos - item->data.int_array.size;
				 item->memory = (void *)item->data.int_array.array;
                 break;
			 }
         case TT_BINDING :
             item->data.binding.sym = free_storage2binding (stored, &pos, &(item->data.binding.context), &(item->data.binding.mods));
             item->memory = (void *)item->data.binding.sym;
             break;
         case TT_CURSOR:
             item->data.cursor = free_storage2cursor (stored, &pos);
             item->memory = (void *)item->data.cursor;
			 break;
        }
		return 1;
	}
	return 0;
}

Bool 
ReadCompositeFlagsConfigItem( void *struct_ptr, ptrdiff_t flags_field_offset, FreeStorageElem * stored )
{
	if( struct_ptr && stored && flags_field_offset != 0 ) 
	{
		FreeStorageElem *pCurr = stored->sub ; 
		ASFlagType result = 0 ; 

		while( pCurr ) 
		{
			set_flags(result, pCurr->term->flags_on);
			clear_flags(result, pCurr->term->flags_off);
			pCurr = pCurr->next ; 
		}
		*((ASFlagType*)(struct_ptr+flags_field_offset)) = result ; 
		return True;
	}
	return False;
}


Bool
ReadConfigItemToStruct( void *struct_ptr, ptrdiff_t set_flags_offset, FreeStorageElem * stored ) 
{
	void *data_ptr = struct_ptr ;
	Bool handled = False;

	if( data_ptr == NULL || stored == NULL ) 
		return False ;

	if( stored->term->type == TT_FLAG && stored->term->sub_syntax )
	{
		handled = ReadCompositeFlagsConfigItem( struct_ptr, stored->term->struct_field_offset, stored );		
	}else if( stored->term->struct_field_offset != 0 )	
	{
		int index = 0 ;
		Bool handled = True;
		ConfigItem item ;
		data_ptr += stored->term->struct_field_offset ;
		
		item.memory = NULL ; 

		if( !ReadConfigItem( &item, stored ) )
			return False ;

        if (get_flags(stored->term->flags, TF_INDEXED))
			index =	item.index ;
	
		handled = True ;		
		switch (item.type)
		{
		 case TT_FLAG: 		((Bool*) data_ptr)[index] = item.data.flag;		 break;
		 case TT_INTEGER:	((int*)  data_ptr)[index] = item.data.integer;	 break;
		 case TT_UINTEGER:	((unsigned int*)  data_ptr)[index] = item.data.integer;	 break;
         case TT_BITLIST :	((int*)  data_ptr)[index] = item.data.integer;	 break;
         case TT_OPTIONAL_PATHNAME:
         case TT_COLOR:
		 case TT_FONT:
		 case TT_FILENAME:
		 case TT_TEXT:
		 case TT_PATHNAME:
		 case TT_QUOTED_TEXT:
						 	set_string(&(((char**)data_ptr)[index]), item.data.string);	 item.memory = NULL ; break;
		 case TT_BOX:		((ASBox*)  data_ptr)[index] = item.data.box;	 break;
		 case TT_GEOMETRY:	((ASGeometry*)  data_ptr)[index] = item.data.geometry;	 break;
		 case TT_FUNCTION:  ((FunctionData**)data_ptr)[index] = item.data.function; item.memory = NULL ; break;
		 case TT_BUTTON:	((ASButton**)data_ptr)[index] = item.data.button; item.memory = NULL; break;
         case TT_CURSOR: 	((ASCursor**)data_ptr)[index] = item.data.cursor; item.memory = NULL ; break;

		 case TT_INTARRAY:	
         case TT_BINDING :	handled = False ; break;

		 	default : handled = False ;
        }
		ReadConfigItem(&item, NULL);
	}

	if( handled && set_flags_offset > 0 ) 
	{
		ASFlagType *set_flags_ptr = struct_ptr+set_flags_offset ;

		set_flags( *set_flags_ptr, stored->term->flags_on );
		clear_flags( *set_flags_ptr, stored->term->flags_off );
	}

	return handled ;		
}

int
ReadFlagItem (unsigned long *set_flags, unsigned long *flags, FreeStorageElem * stored, flag_options_xref * xref)
{
	if (stored && xref)
	{
		Bool          value = True;

        if (stored->term->type != TT_FLAG || 
			get_flags (stored->term->flags, (TF_INDEXED|TF_DIRECTION_INDEXED)) ||
			stored->sub != NULL )
			return 0;

		if (stored->argc >= 1)
			value = (atol (stored->argv[0]) > 0);

		while (xref->flag != 0)
		{
            if (xref->id_on == stored->term->id || xref->id_off == stored->term->id )
            {
                if (set_flags)
                    set_flags (*set_flags, xref->flag);
                if (flags)
                {
                    if ((value && xref->id_on == stored->term->id) ||
                        (!value && xref->id_off == stored->term->id))
                        set_flags (*flags, xref->flag);
                    else
                        clear_flags (*flags, xref->flag);
                }
            }
			xref++;
		}
        return 1;
	}
	return 0;
}

int
ReadFlagItemAuto (void *config_struct, ptrdiff_t default_set_flags_offset, FreeStorageElem * stored, flag_options_xref * xref)
{
	if (stored && xref )
	{
		Bool          value = True;

        if (stored->term->type != TT_FLAG || 
			get_flags (stored->term->flags, (TF_INDEXED|TF_DIRECTION_INDEXED)) ||
			stored->sub != NULL )
			return 0;

		if (stored->argc >= 1)
			value = (atol (stored->argv[0]) > 0);

		while (xref->flag != 0)
		{
            if (xref->id_on == stored->term->id || xref->id_off == stored->term->id )
            {
				unsigned long *flags = (xref->flag_field_offset>0)?config_struct+xref->flag_field_offset:NULL;
				unsigned long *set_flags  = (xref->set_flag_field_offset>0)?config_struct+xref->set_flag_field_offset:
											(default_set_flags_offset>0)?config_struct+default_set_flags_offset:NULL ;
                
				if (set_flags)
                    set_flags (*set_flags, xref->flag);
                if (flags)
                {
                    if ((value && xref->id_on == stored->term->id) ||
                        (!value && xref->id_off == stored->term->id))
                        set_flags (*flags, xref->flag);
                    else
                        clear_flags (*flags, xref->flag);
                }
            }
			xref++;
		}
        return 1;
	}
	return 0;
}


/****************************************************************************/
/* D2F:  Datatype to FreeStorage conversion routines :			    */
/****************************************************************************/


/* helper functions for writing config */

char         *
MyIntToString (int value)
{
	int           charnum, test = value;
	char         *string;

	for (charnum = (test < 0) ? 1 : 0; test != 0 ; charnum++)
		test /= 10;
	if (charnum == 0)
		charnum++;
	charnum++;
	string = (char *)safemalloc (charnum);
	sprintf (string, "%d", value);
	return string;
}

FreeStorageElem **
Integer2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, int *index, int value, int id)
{
	FreeStorageElem *new_elem = AddFreeStorageElem (syntax, tail, NULL, id);

	if (new_elem)
	{
        char *str[2] ;
        int pos = 0, len = 0;

        if( index )
        {
            str[pos++] = string_from_int (*index);
            len = strlen(str[0]);
        }
        str[pos] = string_from_int (value);
        len += strlen(str[pos++]);

        new_elem->argc = pos;
        new_elem->argv = (char **)safecalloc (pos, sizeof (char *));

        new_elem->argv[0] = safemalloc(len+2);
        strcpy( new_elem->argv[0], str[0] );
        if( pos > 1 )
        {
            new_elem->argv[1] = new_elem->argv[0]+strlen(str[0])+1;
            strcpy( new_elem->argv[1], str[1] );
        }

		tail = &(new_elem->next);

	if( index )
	    free(str[1]);
	free(str[0]);

	}
	return tail;
}

FreeStorageElem **
Flag2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, int id)
{
	FreeStorageElem *new_elem = AddFreeStorageElem (syntax, tail, NULL, id);

	if (new_elem)
		tail = &(new_elem->next);

	return tail;
}


FreeStorageElem **
Flags2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail,
				   flag_options_xref * xref, unsigned long set_flags, unsigned long flags)
{
	if (xref)
	{
		while (xref->flag)
		{
			if (get_flags (set_flags, xref->flag))
				tail = Flag2FreeStorage (syntax, tail, get_flags (flags, xref->flag) ? xref->id_on : xref->id_off);

			xref++;
		}
	}
	return tail;
}


FreeStorageElem **
Strings2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, char **strings, unsigned int num, int id)
{
	FreeStorageElem *new_elem;

	if (num == 0 || strings == NULL)
		return tail;
	if ((new_elem = AddFreeStorageElem (syntax, tail, NULL, id)) != NULL)
	{
		new_elem->argc = num;
		new_elem->argv = (char **)safemalloc (sizeof (char *) * num);

		if (num == 1)
			new_elem->argv[0] = mystrdup (*strings ? *strings : "");
		else
		{
			int           len = 0;
			register int  i;
			register char *src, *dst;

			for (i = 0; i < num; i++)
			{
				if (strings[i])
					len += strlen (strings[i]);
				len++;
			}
			dst = (char *)safemalloc (len);
			for (i = 0; i < num; i++)
			{
				new_elem->argv[i] = dst;
				if (strings[i])
				{
					src = strings[i];
					while (*src != '\0')
						*(dst++) = *(src++);
				}
				*(dst++) = '\0';
			}
		}
		tail = &(new_elem->next);
	}
	return tail;
}


FreeStorageElem **
QuotedString2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, int *index, char *string, int id)
{
	FreeStorageElem *new_elem;

	if ((new_elem = AddFreeStorageElem (syntax, tail, NULL, id)) != NULL)
	{
		int           len = 0;
		register char *ptr, *trg;
		char         *idx_string = NULL;

		new_elem->argc = 1;
		if (index)
		{
			idx_string = MyIntToString (*index);
			len = strlen (idx_string);
			new_elem->argc++;
		}
		new_elem->argv = (char **)safemalloc (sizeof (char *) * new_elem->argc);

		if (string)
			for (ptr = string; *ptr; ptr++)
			{
				if (*ptr == '"')
					len++;
				len++;
			}
		new_elem->argv[0] = (char *)safemalloc (1 + 1 + len + 1 + 1);
		trg = new_elem->argv[0];
		if ((ptr = idx_string) != NULL)
		{
			while (*ptr)
				*(trg++) = *(ptr++);
			*(trg++) = '\0';
			new_elem->argv[1] = trg;
			free (idx_string);
		}
		*(trg++) = '"';
		if ((ptr = string) != NULL)
			while (*ptr)
			{
				if (*ptr == '"')
					*(trg++) = '\\';
				*(trg++) = *(ptr++);
			}
		*(trg++) = '"';
		*trg = '\0';

		tail = &(new_elem->next);
	}
	return tail;
}


void
print_signed_value (char *ptr, int value, Bool negative)
{
	if (value == 0)
	{
		if (negative)
			*(ptr++) = '-';
		else
			*(ptr++) = '+';
		*(ptr++) = '0';
		*ptr = '\0';
	} else
		sprintf (ptr, "%+d", value);

}

char         *
add_sign (char *ptr, Bool negative)
{
	if (ptr && *ptr != '-' && *ptr != '+')
	{
		register char *src, *dst;

		ptr = realloc (ptr, 1 + strlen (ptr) + 1);
		src = ptr + strlen (ptr);
		dst = src + 1;
		while (src >= ptr)
			*(dst--) = *(src--);
		*dst = negative ? '-' : '+';
	}
	return ptr;
}

FreeStorageElem **
Geometry2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, ASGeometry * geometry, int id)
{
	char         *geom_string;

	if (geometry)
	{
		register char *ptr;

		ptr = geom_string = safemalloc (128);
		if (geometry->flags & WidthValue)
		{
			sprintf (ptr, "%d", geometry->width);
			ptr += strlen (ptr);
		}
		if (geometry->flags & HeightValue)
		{
			sprintf (ptr, "x%d", geometry->height);
			ptr += strlen (ptr);
		}
		if (geometry->flags & XValue)
		{
			print_signed_value (ptr, geometry->x, (geometry->flags & XNegative));
			ptr += strlen (ptr);
		}
		if (geometry->flags & YValue)
		{
			print_signed_value (ptr, geometry->y, (geometry->flags & YNegative));
			ptr += strlen (ptr);
		}
		tail = Strings2FreeStorage (syntax, tail, &geom_string, 1, id);
		free (geom_string);
	}
	return tail;
}

FreeStorageElem **
StringArray2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail,
						 char **strings, int index1, int index2, int id, char *iformat)
{
	FreeStorageElem *new_elem = NULL;
	int           i;
	char          ind_str[MAXLINELENGTH];
	TermDef      *pterm = FindTerm (syntax, TT_ANY, id);

	if (strings && pterm)
		for (i = 0; i < index2 - index1 + 1; i++)
		{
			if (strings[i] == NULL)
				continue;
			if ((new_elem = AddFreeStorageElem (syntax, tail, pterm, id)) == NULL)
				continue;
			new_elem->argc = 2;
			new_elem->argv = CreateStringArray (2);
			sprintf (ind_str, (iformat == NULL) ? "%d" : iformat, i + index1);
			new_elem->argv[0] = (char *)safemalloc (strlen (ind_str) + 1 + strlen (strings[i]) + 1);
			strcpy (new_elem->argv[0], ind_str);
			new_elem->argv[1] = new_elem->argv[0] + strlen (ind_str) + 1;
			strcpy (new_elem->argv[1], strings[i]);
			tail = &(new_elem->next);
		}
	return tail;
}

FreeStorageElem **
Path2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, int *index, char *path, int id)
{
	if (path)
	{
		register char *ptr;

		for (ptr = path; *ptr; ptr++)
			if (isspace ((int)*ptr))
				return QuotedString2FreeStorage (syntax, tail, index, path, id);
	}
	if (index)
		return StringArray2FreeStorage (syntax, tail, &path, *index, *index, id, NULL);
	return Strings2FreeStorage (syntax, tail, &path, 1, id);
}



FreeStorageElem **
ASButton2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, int index, ASButton * b, int id)
{
#if 0
    TermDef      *pterm = FindTerm (syntax, TT_ANY, id);
	char         *list[ASB_StateCount+1];
	char         *string;
	char         *ind_str;

	if (pterm && b)
	{
		int i ;

		for( i =0; i < ASB_StateCount ; ++i )
            list[i] = b->shapes[i]? b->shapes[i]: _as_nothing_ ;
		list[i] = NULL ;

		ind_str = string_from_int (index);
		string = list2comma_string (list);
		if (ind_str != NULL)				   /* list of icons could be blank */
		{
			FreeStorageElem *new_elem = NULL;

			if ((new_elem = AddFreeStorageElem (syntax, tail, pterm, id)) != NULL)
			{
				new_elem->argc = string ? 2 : 1;
				new_elem->argv = CreateStringArray (new_elem->argc);
				new_elem->argv[0] = (char *)safemalloc (strlen (ind_str) + 1 + (string ? strlen (string) + 1 : 0));
				strcpy (new_elem->argv[0], ind_str);
				if (string)
				{
					new_elem->argv[1] = new_elem->argv[0] + strlen (ind_str) + 1;
					strcpy (new_elem->argv[1], string);
				}
				tail = &(new_elem->next);
			}
		}
		if (string)
			free (string);
		if (ind_str)
			free (ind_str);
	}
#endif
	return tail;
}

FreeStorageElem **
Box2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, ASBox * box, int id)
{
	char         *box_strings[4];
	int           i = 0;

	if (box)
	{
		if (box->flags & LeftValue)
		{
			box_strings[i++] = add_sign (string_from_int (box->left), box->flags & LeftNegative);
			if (box->flags & TopValue)
			{
				box_strings[i++] = add_sign (string_from_int (box->top), box->flags & TopNegative);
				if (box->flags & RightValue)
				{
					box_strings[i++] = add_sign (string_from_int (box->right), box->flags & RightNegative);
					if (box->flags & BottomValue)
						box_strings[i++] = add_sign (string_from_int (box->bottom), box->flags & BottomNegative);
				}
			}
			tail = Strings2FreeStorage (syntax, tail, box_strings, i, id);
			while (--i >= 0)
				free (box_strings[i]);
		}
	}
	return tail;
}

FreeStorageElem **
Binding2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, char *sym, int context, int mods, int id)
{
    char         *strings[3];
    char         context_string[sizeof(int)*8+1+1] ;
    char         mods_string[sizeof(int)*8+1+1] ;

    if (sym)
	{
        strings[0] = sym;
		memset( context_string, ' ', sizeof(int)*8+1 );
		context_string[sizeof(int)*8+1] = '\0' ;
		memset( mods_string, ' ', sizeof(int)*8+1 );
		mods_string[sizeof(int)*8+1] = '\0' ;
		if( context == C_ALL )
		{
			context_string[0] ='A' ;
			context = 0 ;
		}
        context2string( &(context_string[0]), context, as_contexts, True );

		strings[1] = &(context_string[0]);
		if( mods == 0 )
		{
			mods_string[0] ='N' ;
		}else if( mods == AnyModifier )
		{
			mods_string[0] ='A' ;
			mods = 0 ;
		}
        context2string( &(mods_string[0]), mods, key_modifiers, True );
        strings[2] = &(mods_string[0]);

        tail = Strings2FreeStorage (syntax, tail, strings, 3, id);
    }
	return tail;
}

FreeStorageElem **
ASCursor2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, int index, ASCursor *c, int id)
{
	TermDef      *pterm = FindTerm (syntax, TT_ANY, id);
	char         *ind_str;

    if (pterm && c)
	{
        if( c->image_file == NULL || c->mask_file == NULL)
            return tail;

        ind_str = string_from_int (index);

        if (ind_str != NULL)
		{
			FreeStorageElem *new_elem = NULL;

			if ((new_elem = AddFreeStorageElem (syntax, tail, pterm, id)) != NULL)
			{
                new_elem->argc = 3;
                new_elem->argv = CreateStringArray (3);
                new_elem->argv[0] = (char *)safemalloc (strlen (ind_str) + 1 + strlen (c->image_file) + 1 + strlen (c->mask_file) + 1 );
				strcpy (new_elem->argv[0], ind_str);
                new_elem->argv[1] = new_elem->argv[0] + strlen (ind_str) + 1;
                strcpy (new_elem->argv[1], c->image_file);
                new_elem->argv[2] = new_elem->argv[1] + strlen (c->image_file) + 1;
                strcpy (new_elem->argv[1], c->mask_file);
                tail = &(new_elem->next);
			}
            free (ind_str);
        }
    }
	return tail;
}

FreeStorageElem **
Bitlist2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, long bits, int id)
{
    TermDef      *pterm = FindTerm (syntax, TT_ANY, id);
    FreeStorageElem *new_elem = NULL;
    register int i ;
    int count = 0;
    char *ptr ;

    if ((new_elem = AddFreeStorageElem (syntax, tail, pterm, id)) != NULL)
    {
        new_elem->argv =  safecalloc( sizeof(long)*8, sizeof( char*) );
        ptr = safemalloc( sizeof(long)*8*3 );
        for( i = 0 ; i < sizeof(long)*8 ; i++ )
            if( bits & (0x01<<i) )
            {
                new_elem->argv[count++] = ptr ;
                if( i > 10 )
                    *(ptr++) = '0'+(i/10) ;
                *(ptr++) = '0'+i%10 ;
                *(ptr++) = '\0' ;
            }
       new_elem->argc = count  < sizeof(long)*8 ? count : 0 ;
       tail = &(new_elem->next);
    }
    return tail;
}

void
init_asgeometry (ASGeometry * geometry)
{
	geometry->flags = XValue | YValue;
    geometry->x = geometry->y = 0;
    geometry->width = geometry->height = 1;
}



/* Note: Func2FreeStorage has been moved into libASConfig as it is too
 * dependant on FuncSyntax, defined there
 */
