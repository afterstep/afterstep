#ifndef REGEXP_HEADER_FILE_INCLUDED
#define REGEXP_HEADER_FILE_INCLUDED
/************************************************************************/
/************************************************************************/
/*		regexp compiler and datastructures interface		*/
/************************************************************************/

struct reg_exp;

typedef struct wild_reg_exp
{
  unsigned char *raw;

  struct reg_exp *head, *tail, *longest;
  unsigned char max_size, hard_total, soft_total, wildcards_num;

}
wild_reg_exp;

wild_reg_exp *compile_wild_reg_exp (const char *pattern);
void print_wild_reg_exp (wild_reg_exp * wrexp);

/************************************************************************/
/************************************************************************/
/*			Search and sorting methods 			*/
/************************************************************************/

#define DIR_LEFT	(0x01<<0)
#define DIR_RIGHT	(0x01<<1)
#define DIR_BOTH	(DIR_LEFT|DIR_RIGHT)

/* returns 0 if we have a match - -1 if we have to keep searching, 1 - error */
int match_wild_reg_exp (char *string, wild_reg_exp * wrexp);
int compare_wild_reg_exp (wild_reg_exp * wrexp1, wild_reg_exp * wrexp2);

/************************************************************************/

#endif /* REGEXP_HEADER_FILE_INCLUDED */
