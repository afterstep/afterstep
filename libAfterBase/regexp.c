/*
 * Copyright (c) 2000 Sasha Vasko <sasha at aftercode.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "astypes.h"
#include "mystring.h"
#include "safemalloc.h"
#include "regexp.h"
#include "audit.h"

/************************************************************************/
/************************************************************************/
/*			regexp compiler and datastructures		*/
/************************************************************************/

typedef struct reg_exp_multichar_part
{
	unsigned char code[254];
	unsigned char size;
}
reg_exp_multichar_part;


typedef struct reg_exp_sym
{
	unsigned char negate;					   /* if we have [!blahblah] */
	/* in worst case scenario code should not occupy more then 2*255 characters */
	unsigned char code[512];
	unsigned short size;
}
reg_exp_sym;

typedef struct reg_exp
{
	struct reg_exp *prev, *next;

	short int     left_offset, right_offset;   /* min number of characters that we have to
											      leave on each size of the string in question
											      for other patterns to be able to match */
	short int     left_fixed, right_fixed;	   /* indicates if we have a * on left or
											      right side and therefore flexible
											      with our position */

	short int     lead_len;					   /* -1 for '*' */
	/* number of symbols : */
	unsigned char size;						   /* upto 255 characters in regexp */
	/* symbol can be:
	   - single character    : c
	   - set of characters   : [ccccccc]
	   - range of characters : [r-r]
	   more then that, it can be negated : [!cccccc] or [!r-r]
	   or it can be combined         : [cccc,r-r] or [!ccccc,r-r]

	   methinks we should pack them all together like so:

	   ccccc0x01rr0x01rr0x01rrccccc0x00.....
	   where ccccc - are single characters that are compared one-to-one
	   and rr - represent range on characters like r-r.
	   0x01 denotes range
	   0x00 denotes end of the symbol.
	   so we'll store them in single character array,
	   plus another array will store negation indicators.
	 */

	unsigned char *symbols;
	unsigned char *negation;

	/* in Booer-Moor algorithm we need to use a skip table  -
	   indicating where we should go if character mismatches any symbol
	   (skip offset for each character in alphabeth)
	 */
	unsigned char skip[256];				   /* skip table */

}
reg_exp;

unsigned char
parse_singlechar (unsigned char **data, char *reserved)
{
	register unsigned char c = **data;

    if( c != '\0' )
    {
        while (*reserved)
            if ((c == (unsigned char)*(reserved++)))
                return 0x00;

        if (c == '\\')
            c = *(++(*data));

        (*data)++;
    }
	return c;
}

reg_exp_multichar_part *
parse_multichar_part (unsigned char **data)
{
	static reg_exp_multichar_part part;
	unsigned char c;

	if ((c = parse_singlechar (data, "],")) == 0x00)
		return NULL;
	if (**data == '-')
	{										   /* range */
		part.size = 3;
		(*data)++;
		part.code[0] = 0x01;
		part.code[1] = c;
		if ((c = parse_singlechar (data, "],")) == 0x00)
			return NULL;
		part.code[2] = c;
	} else
	{										   /* enumeration */
		part.size = 1;
		part.code[0] = c;
		while ((c = parse_singlechar (data, "],")) != 0x00)
			part.code[part.size++] = c;
	}
	return &part;
}

reg_exp_sym  *
parse_reg_exp_sym (unsigned char **data)
{											   /* this is a non-reentrant function in order to conserve memory */
	static reg_exp_sym sym;

	sym.negate = 0;
	sym.size = 0;

	if (**data == '[')
	{
		unsigned char *ptr = (*data) + 1;

		if (*ptr == '!')
		{
			sym.negate = 1;
			ptr++;
		}
		do
		{
			reg_exp_multichar_part *p_part;

			p_part = parse_multichar_part (&ptr);
			if (p_part == NULL)
				break;
			memcpy (&(sym.code[sym.size]), p_part->code, p_part->size);
			sym.size += p_part->size;
			if (*ptr == ',')
				ptr++;
		}
		while (*ptr != ']');
		*data = ptr + 1;
	} else if ((sym.code[sym.size++] = parse_singlechar (data, "*?")) == 0x00)
		return NULL;

	if (sym.size == 0)
		return NULL;

	sym.code[sym.size++] = 0x00;			   /* terminator */
	return &sym;
}

void
fix_skip_table (reg_exp * rexp)
{
	register int  i;
	unsigned char *ptr;
	unsigned char c;

	if (rexp == NULL)
		return;
	ptr = rexp->symbols;

	for (i = 0; i < 256; i++)
		rexp->skip[i] = rexp->size;
	/* symbols are stored in opposite direction - so skip is increasing */
	for (i = 0; i < rexp->size; i++)
	{
		while (*ptr)
		{
			if (*ptr == 0x01)				   /* multichar range */
			{
				ptr++;
				ptr++;
				for (c = *(ptr - 1); c <= *ptr; c++)
					rexp->skip[c] = i;
			} else
				rexp->skip[*ptr] = i;
			ptr++;
		}
		ptr++;
	}
}

unsigned char *
optimize_reg_exp_sym (unsigned char *trg, reg_exp_sym * p_sym)
{
/* there could be up to 255 possible charcters :) - that excludes 0x00 */
	unsigned char scale[256];
	unsigned char *ptr = p_sym->code;
	unsigned char r_start, r_end;

	memset (scale, 0x00, 256);
	while (*ptr)
	{
		if (*ptr == 0x01)
		{
			r_start = *(++ptr);
			ptr++;
			/* we need to doublecheck if ranges are really min-max */
			if (r_start > *ptr)
			{
				r_end = r_start;
				r_start = *ptr;
			} else
				r_end = *ptr;

			if (r_start > 0x00 && r_end > 0x00)
			{
				while (r_start < r_end)
					scale[r_start++] = 1;
				/* to avoid overflow and endless loop doing it as separate step */
				scale[r_end] = 1;
			}
		} else
			scale[*ptr] = 1;
		ptr++;
	}
	/* we want to reduce ranges by excluding what is overlapping */
	/* then reduce single characters by removing those that fall into ranges */
	/* and maybe join them all together if they form continuous sequences */
	/* to do that simply scan through the scale */
	r_start = r_end = 0x00;
	/* skipping 0x00 and 0x01 as special characters */
	for (r_end = 2; r_end < 255; r_end++)
	{
		if (scale[r_end] == 0x00)
		{
			if (r_start != 0x00)
			{
				if (r_end - 1 > r_start + 1)
					*(trg++) = 0x01;
				*(trg++) = r_start;
				if (r_end - 1 > r_start)
					*(trg++) = r_end - 1;
				r_start = 0x00;
			}
		} else if (r_start == 0x00)
			r_start = r_end;
	}
	/* final data may have got forgotten since last scale element may not be 0x00 */
	if (r_start != 0x00)
	{
		if (scale[r_end] == 0x00)
			r_end--;
		if (r_end > r_start + 1)
			*(trg++) = 0x01;
		*(trg++) = r_start;
		if (r_end > r_start)
			*(trg++) = r_end;
	}
	/* Note also that this procedure will arrange all the codes in ascending order */
	/* so later on when we trying to match it and code in question is less then
	   any code - we can stop and make decision that we have no match :) */
	*(trg++) = 0x00;
	return trg;
}

reg_exp      *
parse_reg_exp (short int lead_len, unsigned char **data)
{											   /* this is a non-reentrant function in order to conserve memory */
	reg_exp      *trg;
	reg_exp_sym  *p_sym;

	p_sym = parse_reg_exp_sym (data);
	if (lead_len == 0 && p_sym == NULL)
		return NULL;

	trg = safemalloc (sizeof (reg_exp));
	memset (trg, 0x00, sizeof (reg_exp));

	trg->lead_len = lead_len;
	trg->size = 0;

	if (p_sym)
	{
		int           len = strlen ((char*)*data) + p_sym->size + 1;
		unsigned char *sym_buf, *sym_next, *neg_buf;
		register unsigned char *src, *dst;
		int           i;

		/* in worst case scenario symbols will occupy 2*n where n -
		   length of the remaining data.
		   at the same time negation will occupy only n bytes max anytime :
		 */

		sym_next = sym_buf = safemalloc (len * 2);
		neg_buf = safemalloc (len);

		/* lets avoid recursion a little bit where we can - */
		/* since reg_exp is nothing but a row of symbols :  */
		do
		{
			sym_next = optimize_reg_exp_sym (sym_next, p_sym);
			neg_buf[(trg->size)++] = p_sym->negate;
		}
		while ((p_sym = parse_reg_exp_sym (data)) != NULL);

		dst = trg->symbols = safemalloc (sym_next - sym_buf);
		trg->negation = safemalloc (trg->size);
		sym_next--;
		/* we have to store symbols in reverse order, since
		   Booyer-Moor algorithm scans in opposite direction */
		for (i = 0; i < trg->size; i++)
		{
			sym_next--;
			while (sym_next > sym_buf && *sym_next)
				sym_next--;
			src = (*sym_next) ? sym_next : sym_next + 1;
			while (*src)
				*(dst++) = *(src++);
			*(dst++) = *src;

			trg->negation[i] = neg_buf[trg->size - i - 1];
		}

		free (sym_buf);
		free (neg_buf);
	} else
	{
		trg->symbols = NULL;
		trg->negation = NULL;
	}

	fix_skip_table (trg);

	return trg;
}

short int
parse_wild (unsigned char **data)
{											   /* we go through any sequence of the '*' and '?' characters */
	short int     count = 0;
	register unsigned char *ptr = *data;

	do
	{
		if (*ptr == '*')
			count = -1;
		else if (*ptr == '?')
		{
			if (count >= 0)
				count++;
		} else
			break;
	}
	while (*(++ptr));

	*data = ptr;
	return count;
}

wild_reg_exp *
parse_wild_reg_exp (unsigned char **data)
{
	wild_reg_exp *trg;
	reg_exp      *p_reg;
	short int     count = 0;

	if (**data == '\0')
		return NULL;

	/* first comes wildcards (maybe) */
	count = parse_wild (data);

	/* then reg_exp */
	if ((p_reg = parse_reg_exp (count, data)) == NULL)
		return NULL;

	/* then we can find another long wild_reg_exp */
	trg = parse_wild_reg_exp (data);

	if (trg)
	{
		p_reg->next = trg->head;
		if (trg->head)
			trg->head->prev = p_reg;
		p_reg->prev = NULL;
		trg->head = p_reg;

		trg->hard_total += p_reg->size;
		if (p_reg->lead_len >= 0)
			trg->soft_total += p_reg->lead_len;
		else
			trg->wildcards_num++;

		if (trg->max_size < p_reg->size)
		{
			trg->max_size = p_reg->size;
			trg->longest = p_reg;
		}
	} else
	{
		trg = safemalloc (sizeof (wild_reg_exp));
		trg->max_size = p_reg->size;
		trg->longest = p_reg;
		trg->head = trg->tail = p_reg;

		trg->hard_total = p_reg->size;
		trg->soft_total = (p_reg->lead_len > 0) ? p_reg->lead_len : 0;
		trg->wildcards_num = (p_reg->lead_len < 0) ? 1 : 0;
	}
	return trg;
}

char         *
place_singlechar (char *trg, unsigned char c)
{
	if (c == '*' || c == '?' || c == '[' || c == ']' || c == '!' || c == ',' || c == '-')
		*(trg++) = '\\';

	*(trg++) = c;
	return trg;
}

char         *
flatten_wild_reg_exp (wild_reg_exp * wrexp)
{
	int           size = 0;
	reg_exp      *curr;
	int           i;
	unsigned char *ptr;
	char         *trg_start, *trg;

	/* dry run to find out just how much space we need to allocate for our string */
	for (curr = wrexp->head; curr; curr = curr->next)
	{
		size += (curr->lead_len < 0) ? 1 : curr->lead_len;
		ptr = curr->symbols;
		for (i = 0; i < curr->size; i++)
		{
			if (*(ptr + 1) || curr->negation[i])
			{
				size += 2;
				if (curr->negation[i])
					size++;
			}

			for (; *ptr; ptr++)
			{
				if (*ptr == '*' || *ptr == '?' || *ptr == '[' ||
					*ptr == ']' || *ptr == '!' || *ptr == ',' || *ptr == '-')
					size++;					   /* for prepending \ */
				else if (*ptr == 0x01)
					size += 2;				   /* for comma */
				size++;
			}
			ptr++;
		}
	}
	trg = trg_start = safemalloc (size + 1);
	/* now real run */
	for (curr = wrexp->head; curr; curr = curr->next)
	{
		unsigned char *tail;

		if (curr->lead_len < 0)
			*(trg++) = '*';
		else
			for (i = 0; i < curr->lead_len; i++)
				*(trg++) = '?';
		tail = curr->symbols;
		for (i = curr->size - 1; i >= 0; i--)
			while (*(++tail));

		for (i = curr->size - 1; i >= 0; i--)
		{
			char          in_multi;
			int           part_type;

			ptr = tail - 1;

			while (ptr > curr->symbols && *ptr)
				ptr--;
			tail = ptr;
			if (ptr > curr->symbols)
				ptr++;

			if (*(ptr + 1) || curr->negation[i])
			{
				*(trg++) = '[';
				in_multi = ']';
				if (curr->negation[i])
					*(trg++) = '!';
			} else
				in_multi = '\0';

			part_type = (*ptr == 0x01) ? 1 : 0;
			for (; *ptr; ptr++)
			{
				if (*ptr == 0x01)
				{
					if (part_type != 1)
						*(trg++) = ',';
					part_type = 2;
					if (*(++ptr))
					{
						trg = place_singlechar (trg, *ptr);
						if (*(++ptr))
						{
							*(trg++) = '-';
							trg = place_singlechar (trg, *ptr);
						}
					}
				} else
				{
					if (part_type != 0)
						*(trg++) = ',';
					part_type = 0;
					trg = place_singlechar (trg, *ptr);
				}
			}
			if (in_multi)
				*(trg++) = in_multi;
		}
	}
	*(trg) = '\0';
	return trg_start;

}

void
make_offsets (wild_reg_exp * wrexp)
{
	reg_exp      *curr;
	short int     offset, fixed;

	if (wrexp)
	{
		offset = 0;
		fixed = 1;
		for (curr = wrexp->head; curr; curr = curr->next)
		{
			if (curr->lead_len < 0)
				fixed = 0;
			else
				offset += curr->lead_len;
			/* very important: *? prior to this regexp does affect left_fixed !!! */
			curr->left_offset = offset;
			curr->left_fixed = fixed;

			offset += curr->size;
		}
		offset = 0;
		fixed = 1;
		for (curr = wrexp->tail; curr; curr = curr->prev)
		{
			curr->right_offset = offset;
			/* very important: *? prior to this regexp does not affect right_fixed !!! */
			curr->right_fixed = fixed;

			if (curr->lead_len < 0)
				fixed = 0;
			else
				offset += curr->lead_len;

			offset += curr->size;
		}
	}
}

wild_reg_exp *
compile_wild_reg_exp (const char *pattern)
{
	unsigned char *buffer;
	unsigned char *ptr = (unsigned char *)pattern;
	register int  i;
	wild_reg_exp *trg = NULL;

	if (ptr == NULL)
		return NULL;

	for (i = 0; *(ptr + i) != '\0'; i++);
	if (i > 254)
		i = 254;
	buffer = safemalloc (i + 1);

	for (i = 0; i < 253; i++)
	{
		buffer[i] = *(ptr++);
		if (*ptr == '\0')
		{
			buffer[++i] = *ptr;
			break;
		}
	}
	if (*ptr)
	{										   /* lets make sure we don't exceed 255 chars limit */
		if (buffer[252] == '\\' && buffer[251] != '\\')
		{
			buffer[252] = '*';
			buffer[253] = '\0';
		} else
		{
			buffer[253] = '*';
			buffer[254] = '\0';
		}
	}
	ptr = buffer;
	trg = parse_wild_reg_exp (&ptr);
	free (buffer);

	trg->raw = (unsigned char*)flatten_wild_reg_exp (trg);
	make_offsets (trg);
	return trg;
}

void
print_wild_reg_exp (wild_reg_exp * wrexp)
{
	reg_exp      *curr;
	int           i = 0, k;

	if (wrexp == NULL)
		return;

	fprintf (stderr, "wild_reg_exp :{%s}\n", wrexp->raw);
	fprintf (stderr, "    max_size : %d\n", wrexp->max_size);
	fprintf (stderr, "  total size : (hard)%d (soft)%d (wildcards)%d\n{\n",
			 wrexp->hard_total, wrexp->soft_total, wrexp->wildcards_num);
	for (curr = wrexp->head; curr; curr = curr->next)
	{
		fprintf (stderr, "\tregexp #%d:\n\t{\n", i++);

		{
			unsigned char *ptr = curr->symbols;

			fprintf (stderr, "\t\t offsets : (%d<%s>,%d<%s>)\n",
					 curr->left_offset,
					 (curr->left_fixed) ? "fixed" : "not fixed",
					 curr->right_offset, (curr->right_fixed) ? "fixed" : "not fixed");
			fprintf (stderr, "\t\t lead_len = %d\n", curr->lead_len);
			fprintf (stderr, "\t\t size = %d\n", curr->size);
			fprintf (stderr, "\t\t Symbols :\n\t\t{\n\t\t\t");
			for (k = 0; k < curr->size; k++)
			{
				fprintf (stderr, "#%d->", k);

				while (*ptr)
					fprintf (stderr, "%c", *(ptr++));

				if (curr->negation[k])
					fprintf (stderr, "\t\tNegated");
				fprintf (stderr, "\n\t\t\t");
				ptr++;
			}
			fprintf (stderr, "\n\t\t}\n");
		}

		fprintf (stderr, "\t}\n");
	}
	fprintf (stderr, "}\n");
}

/************************************************************************/
/************************************************************************/
/*			Search and sorting methods 			*/
/************************************************************************/

int
match_substring (reg_exp * rexp, char *string, size_t len, int dir)
{
	signed int    start = 0, end = len; /* Important - we need signed int here !!! */
	signed int    pos, offset, size;
	register unsigned char *sym, *neg;
	unsigned char c, r1;
	signed int    skip, lead_len;

	if (rexp == NULL)
		return 0;							   /* we've riched the end of the search */
	if (string == NULL)
		return 1;
	size = rexp->size;
	if (len < size)
		return -1;

	lead_len = rexp->lead_len;
	if (size == 0)							   /* all we have is a wildcard */
		if (lead_len < 0 || len == lead_len)
			return 0;

	if (lead_len < 0)
		lead_len = 0;

	start = (dir & DIR_LEFT) ? rexp->left_offset : lead_len;
	end = (dir & DIR_RIGHT) ? len - rexp->right_offset : len;
/*fprintf( stderr, "dir = %d, lead_len = %d, size = %d, start = %d, end = %d, len = %d\n string = {%s}\n", dir, lead_len, size, start, end, len, string );
*/
	if (rexp->left_fixed || (!(dir & DIR_LEFT) && rexp->lead_len >= 0))
	{
		if (rexp->right_fixed)
		{
			if (end - start != size)
				return -1;
		} else
			end = start + size;
	} else if (rexp->right_fixed || !(dir & DIR_RIGHT))
		start = end - size;

	pos = start + size - 1;
/*fprintf( stderr, "adjusted: pos = %d, start = %d, end = %d\n", pos, start, end );
*/
	while (pos < end)
	{
		sym = rexp->symbols;
		neg = rexp->negation;
		c = '\0';
		for (offset = pos; offset >= start; offset--)
		{
			c = string[offset];
			/* try and compare current symbol */
			while (*sym)
			{
				if (*sym == 0x01)
				{
					sym++;
					r1 = *(sym++);
					if (c >= r1 && c <= *sym)
						break;
				} else if (c == *sym)
					break;
				sym++;
			}
			if (*sym == '\0' || *neg == 1)	   /* then we have a mismatch */
				break;
			while (*sym)
				sym++;
			sym++;
			neg++;
		}

		if (offset < start)
		{									   /* we have a match - now try to match other regexps : */
			int           bres = 0;

			if ((dir & DIR_LEFT) && rexp->prev)
				bres = match_substring (rexp->prev, string, start - lead_len, DIR_LEFT);
			if (bres == 0 && (dir & DIR_RIGHT) && rexp->next)
				bres = match_substring (rexp->next, string + pos + 1, len - (pos + 1), DIR_RIGHT);
			if (bres == 0)
				return 0;					   /* hooorrraaaaayyyyy !!!!!!!!!! */
		}
		/* ouch, we have a mismatch ! */
		skip = pos - offset + 1;
		/* additional heuristic here: */
		if (skip < rexp->skip[c])
			skip = rexp->skip[c];

		pos += skip;
		start += skip;
	}
	return -1;
}

int											   /* returns 0 if we have a match - -1 if we have to keep searching, 1 - error */
match_wild_reg_exp (char *string, wild_reg_exp * wrexp)
{
	size_t        len;

	if (wrexp == NULL || string == NULL)
		return 1;
	if (wrexp->longest == NULL)
		return -1;							   /* empty regexp matches nothing */
	len = strlen (string);
	if (len < wrexp->hard_total + wrexp->soft_total)
		return -1;							   /* string is too short to match criteria */
	return match_substring (wrexp->longest, string, len, DIR_BOTH);
}

int                                            /* returns 0 if we have a match - -1 if we have to keep searching, 1 - error */
match_string_list (char **list, int max_elem, wild_reg_exp * wrexp)
{
    int res = -1;
    register int i ;

    if (wrexp == NULL || list == NULL)
        return 1;
    if (wrexp->longest == NULL)  /* empty regexp matches nothing */
        return -1;

    for( i = 0 ; i < max_elem ; i++ )
    {
        if( list[i] == NULL ) break ;
        else
        {
            int len = strlen (list[i]);
            if (len >= wrexp->hard_total + wrexp->soft_total)/* string has sufficient length */
                if( (res = match_substring (wrexp->longest, list[i], len, DIR_BOTH)) >= 0 )
                    break ;
        }
    }
    return res;
}


int
compare_wild_reg_exp (wild_reg_exp * wrexp1, wild_reg_exp * wrexp2)
{
	int           strcmp_res, cmp_res;

	if (wrexp1 == wrexp2)
		return 0;
	if (wrexp1 == NULL)
		return -1;
	if (wrexp2 == NULL)
		return 1;
	strcmp_res = strcmp ((const char*)(wrexp1->raw), (const char*)(wrexp2->raw));
	if (strcmp_res == 0)
		return 0;
	cmp_res = wrexp1->hard_total - wrexp2->hard_total;
	if (cmp_res == 0)
	{
		cmp_res = wrexp1->soft_total - wrexp2->soft_total;
		if (cmp_res == 0)
		{									   /*  the more wildcrads we have - the less strict pattern it is
											      so here we use different way then above : */
			cmp_res = -wrexp1->wildcards_num + wrexp2->wildcards_num;
			if (cmp_res == 0)
				cmp_res = strcmp_res;
		}
	}
	return cmp_res;
}

/************************************************************************/

void
destroy_wild_reg_exp (wild_reg_exp * wrexp)
{
	reg_exp      *curr, *next = NULL;

	if (wrexp == NULL)
		return;

	for (curr = wrexp->head; curr; curr = next)
	{
		next = curr->next;
		if (curr->symbols)
			free (curr->symbols);
		if (curr->negation)
			free (curr->negation);
		free (curr);
	}

	if (wrexp->raw)
		free (wrexp->raw);
	free (wrexp);
}

/************************************************************************/
/* Older code: very ugly but simple and may come in handy someplace :   */
/************************************************************************/
/************************************************************************
 *	Does `string' match `pattern'? '*' in pattern matches any sub-string
 *	(including the null string) '?' matches any single char. For use
 *	by filenameforall. Note that '*' matches across directory boundaries
 *
 *      This code donated by  Paul Hudson <paulh@harlequin.co.uk>
 *      It is public domain, no strings attached. No guarantees either.
 *		Modified by Emanuele Caratti <wiz@iol.it>
 *
 *************************************************************************/

int
matchWildcards (const char *pattern, const char *string)
{
	if (pattern == NULL)
		return 1;
	else if (*pattern == '*' && !*(pattern + 1))
		return 1;

	if (string == NULL)
		return 0;

	while (*string && *pattern)
	{
		if (*pattern == '?')
		{
			/* match any character */
			pattern++;
			string++;
		} else if (*pattern == '*')
		{
			/* see if the rest of the pattern matches any trailing substring
			 * of the string. */
			pattern++;
			if (*pattern == 0)
			{
				return 1;					   /* trailing * must match rest */
			}
			while (1)
			{
				while (*string && (*string != *pattern))
					string++;
				if (!*string)
					return 0;
				if (matchWildcards (pattern, string))
					return 1;
				string++;
			}

		} else
		{
			if (*pattern == '\\')
				pattern++;					   /* has strange, but harmless effects if the last
											      * character is a '\\' */
			if (*pattern++ != *string++)
			{
				return 0;
			}
		}
	}
	if ((*pattern == 0) && (*string == 0))
		return 1;
	if ((*string == 0) && (strcmp (pattern, "*") == 0))
		return 1;
	return 0;
}

