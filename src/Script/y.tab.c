
/*  A Bison parser, made from Compiler/bisonin
   by  GNU Bison version 1.25
 */

int yylex (void);

#define YYBISON 1		/* Identify Bison output.  */

#define	STR	258
#define	GSTR	259
#define	VAR	260
#define	NUMBER	261
#define	WINDOWTITLE	262
#define	WINDOWSIZE	263
#define	WINDOWPOSITION	264
#define	FONT	265
#define	FORECOLOR	266
#define	BACKCOLOR	267
#define	SHADCOLOR	268
#define	LICOLOR	269
#define	OBJECT	270
#define	INIT	271
#define	PERIODICTASK	272
#define	MAIN	273
#define	END	274
#define	PROP	275
#define	TYPE	276
#define	SIZE	277
#define	POSITION	278
#define	VALUE	279
#define	VALUEMIN	280
#define	VALUEMAX	281
#define	TITLE	282
#define	SWALLOWEXEC	283
#define	ICON	284
#define	FLAGS	285
#define	WARP	286
#define	WRITETOFILE	287
#define	HIDDEN	288
#define	CANBESELECTED	289
#define	NORELIEFSTRING	290
#define	CASE	291
#define	SINGLECLIC	292
#define	DOUBLECLIC	293
#define	BEG	294
#define	POINT	295
#define	EXEC	296
#define	HIDE	297
#define	SHOW	298
#define	CHFORECOLOR	299
#define	CHBACKCOLOR	300
#define	GETVALUE	301
#define	CHVALUE	302
#define	CHVALUEMAX	303
#define	CHVALUEMIN	304
#define	ADD	305
#define	DIV	306
#define	MULT	307
#define	GETTITLE	308
#define	GETOUTPUT	309
#define	GETASOPTION	310
#define	SETASOPTION	311
#define	STRCOPY	312
#define	NUMTOHEX	313
#define	HEXTONUM	314
#define	QUIT	315
#define	LAUNCHSCRIPT	316
#define	GETSCRIPTFATHER	317
#define	SENDTOSCRIPT	318
#define	RECEIVFROMSCRIPT	319
#define	GET	320
#define	SET	321
#define	SENDSIGN	322
#define	REMAINDEROFDIV	323
#define	GETTIME	324
#define	GETSCRIPTARG	325
#define	IF	326
#define	THEN	327
#define	ELSE	328
#define	FOR	329
#define	TO	330
#define	DO	331
#define	WHILE	332
#define	BEGF	333
#define	ENDF	334
#define	EQUAL	335
#define	INFEQ	336
#define	SUPEQ	337
#define	INF	338
#define	SUP	339
#define	DIFF	340

#line 1 "Compiler/bisonin"

#include "types.h"

extern int numligne;
ScriptProp *scriptprop;
int nbobj = -1;			/* Nombre d'objets */
int HasPosition, HasType = 0;
TabObj *tabobj;			/* Tableau d'objets, limite=100 */
int TabIdObj[101];		/* Tableau d'indice des objets */
Bloc **TabIObj;			/* TabIObj[Obj][Case] -> bloc attache au case */
Bloc *PileBloc[10];		/* Au maximum 10 imbrications de boucle conditionnelle */
int TopPileB = 0;		/* Sommet de la pile des blocs */
CaseObj *TabCObj;		/* Struct pour enregistrer les valeurs des cases et leur nb */
int CurrCase;
int i;
char **TabNVar;			/* Tableau des noms de variables */
char **TabVVar;			/* Tableau des valeurs de variables */
int NbVar;
long BuffArg[6][20];		/* Les arguments s'ajoute par couche pour chaque fonction imbriquee */
int NbArg[6];			/* Tableau: nb d'args pour chaque couche */
int SPileArg;			/* Taille de la pile d'arguments */
long l;

/* Initialisation globale */
void
InitVarGlob ()
{
  scriptprop = (ScriptProp *) calloc (1, sizeof (ScriptProp));
  scriptprop->x = -1;
  scriptprop->y = -1;
  scriptprop->initbloc = NULL;

  tabobj = (TabObj *) calloc (1, sizeof (TabObj));
  for (i = 0; i < 101; i++)
    TabIdObj[i] = -1;
  TabNVar = NULL;
  TabVVar = NULL;
  NbVar = -1;

  SPileArg = -1;
  scriptprop->periodictasks = NULL;
}

/* Initialisation pour un objet */
void
InitObjTabCase (int HasMainLoop)
{
  if (nbobj == 0)
    {
      TabIObj = (Bloc **) calloc (1, sizeof (long));
      TabCObj = (CaseObj *) calloc (1, sizeof (CaseObj));
    }
  else
    {
      TabIObj = (Bloc **) realloc (TabIObj, sizeof (long) * (nbobj + 1));
      TabCObj = (CaseObj *) realloc (TabCObj, sizeof (CaseObj) * (nbobj + 1));
    }

  if (!HasMainLoop)
    TabIObj[nbobj] = NULL;
  CurrCase = -1;
  TabCObj[nbobj].NbCase = -1;
}

/* Ajout d'un case dans la table TabCase */
/* Initialisation d'un case of: agrandissement de la table */
void
InitCase (int cond)
{
  CurrCase++;

  /* On enregistre la condition du case */
  TabCObj[nbobj].NbCase++;
  if (TabCObj[nbobj].NbCase == 0)
    TabCObj[nbobj].LstCase = (int *) calloc (1, sizeof (int));
  else
    TabCObj[nbobj].LstCase = (int *) realloc (TabCObj[nbobj].LstCase, sizeof (int) * (CurrCase + 1));
  TabCObj[nbobj].LstCase[CurrCase] = cond;

  if (CurrCase == 0)
    TabIObj[nbobj] = (Bloc *) calloc (1, sizeof (Bloc));
  else
    TabIObj[nbobj] = (Bloc *) realloc (TabIObj[nbobj], sizeof (Bloc) * (CurrCase + 1));

  TabIObj[nbobj][CurrCase].NbInstr = -1;
  TabIObj[nbobj][CurrCase].TabInstr = NULL;

  /* Ce case correspond au bloc courant d'instruction: on l'empile */
  PileBloc[0] = &TabIObj[nbobj][CurrCase];
  TopPileB = 0;
}

/* Enleve un niveau d'args dans la pile BuffArg */
void
RmLevelBufArg ()
{
  SPileArg--;
}

/* Fonction de concatenation des n derniers etage de la pile */
/* Retourne les elts trie et depile et la taille */
long *
Depile (int NbLevelArg, int *s)
{
  long *Temp;
  int j;
  int i;
  int size;

  if (NbLevelArg > 0)
    {
      Temp = (long *) calloc (1, sizeof (long));
      size = 0;
      for (i = SPileArg - NbLevelArg + 1; i <= SPileArg; i++)
	{
	  size = NbArg[i] + size + 1;
	  Temp = (long *) realloc (Temp, sizeof (long) * size);
	  for (j = 0; j <= NbArg[i]; j++)
	    {
	      Temp[j + size - NbArg[i] - 1] = BuffArg[i][j];
	    }
	}
      *s = size;
      for (i = 0; i < NbLevelArg; i++)	/* On depile les couches d'arguments */
	RmLevelBufArg ();
      return Temp;
    }
  else
    {
      return NULL;
      *s = 0;
    }
}

/* Ajout d'une commande */
void
AddCom (int Type, int NbLevelArg)
{
  int CurrInstr;


  PileBloc[TopPileB]->NbInstr++;
  CurrInstr = PileBloc[TopPileB]->NbInstr;

  if (CurrInstr == 0)
    PileBloc[TopPileB]->TabInstr = (Instr *) calloc (1, sizeof (Instr) * (CurrInstr + 1));
  else
    PileBloc[TopPileB]->TabInstr = (Instr *) realloc (PileBloc[TopPileB]->TabInstr,
					  sizeof (Instr) * (CurrInstr + 1));
  /* Rangement des instructions dans le bloc */
  PileBloc[TopPileB]->TabInstr[CurrInstr].Type = Type;
  /* On enleve la derniere couche d'argument et on la range dans la commande */

  PileBloc[TopPileB]->TabInstr[CurrInstr].TabArg = Depile (NbLevelArg,
			    &PileBloc[TopPileB]->TabInstr[CurrInstr].NbArg);
}

/* Initialisation du buffer contenant les arguments de la commande courante */
/* Ajout d'une couche d'argument dans la pile */
void
AddLevelBufArg ()
{
  /* Agrandissment de la pile */
  SPileArg++;
  NbArg[SPileArg] = -1;
}

/* Ajout d'un arg dans la couche arg qui est au sommet de la pile TabArg */
void
AddBufArg (long *TabLong, int NbLong)
{
  int i;

  for (i = 0; i < NbLong; i++)
    {
      BuffArg[SPileArg][i + NbArg[SPileArg] + 1] = TabLong[i];
    }
  NbArg[SPileArg] = NbArg[SPileArg] + NbLong;
}

/* Recheche d'un nom de var dans TabVar, s'il n'existe pas il le cree */
/* Retourne un Id */
void
AddVar (char *Name)		/* ajout de variable a la fin de la derniere commande pointee */
{
  int i;

  /* Comparaison avec les variables deja existante */
  for (i = 0; i <= NbVar; i++)
    if (strcmp (TabNVar[i], Name) == 0)
      {
	l = (long) i;
	AddBufArg (&l, 1);
	return;
      }

  if (NbVar > 58)
    {
      fprintf (stderr, "Line %d: too many variables (>60)\n", numligne);
      exit (1);
    }

  /* La variable n'a pas ete trouvee: creation */
  NbVar++;

  if (NbVar == 0)
    {
      TabNVar = (char **) calloc (1, sizeof (long));
      TabVVar = (char **) calloc (1, sizeof (long));
    }
  else
    {
      TabNVar = (char **) realloc (TabNVar, sizeof (long) * (NbVar + 1));
      TabVVar = (char **) realloc (TabVVar, sizeof (long) * (NbVar + 1));
    }

  TabNVar[NbVar] = (char *) strdup (Name);
  TabVVar[NbVar] = (char *) calloc (1, sizeof (char));
  TabVVar[NbVar][0] = '\0';


  /* Ajout de la variable dans le buffer Arg */
  l = (long) NbVar;
  AddBufArg (&l, 1);
  return;
}

/* Ajout d'une constante str comme argument */
void
AddConstStr (char *Name)
{
  /* On cree une nouvelle variable et on range la constante dedans */
  NbVar++;
  if (NbVar == 0)
    {
      TabVVar = (char **) calloc (1, sizeof (long));
      TabNVar = (char **) calloc (1, sizeof (long));
    }
  else
    {
      TabVVar = (char **) realloc (TabVVar, sizeof (long) * (NbVar + 1));
      TabNVar = (char **) realloc (TabNVar, sizeof (long) * (NbVar + 1));
    }

  TabNVar[NbVar] = (char *) calloc (1, sizeof (char));
  TabNVar[NbVar][0] = '\0';
  TabVVar[NbVar] = (char *) strdup (Name);

  /* Ajout de l'id de la constante dans la liste courante des arguments */
  l = (long) NbVar;
  AddBufArg (&l, 1);
}

/* Ajout d'une constante numerique comme argument */
void
AddConstNum (long num)
{

  /* On ne cree pas de nouvelle variable */
  /* On code la valeur numerique afin de le ranger sous forme d'id */
  l = num + 200000;
  /* Ajout de la constante dans la liste courante des arguments */
  AddBufArg (&l, 1);
}

/* Ajout d'une fonction comme argument */
/* Enleve les args de func de la pile, */
/* le concate, et les range dans la pile */
void
AddFunct (int code, int NbLevelArg)
{
  int size;
  long *l;
  int i;

  /* Methode: depiler BuffArg et completer le niveau inferieur de BuffArg */
  l = Depile (NbLevelArg, &size);

  size++;
  if (size == 1)
    l = (long *) calloc (1, sizeof (long));
  else
    {
      l = (long *) realloc (l, sizeof (long) * (size));
      for (i = size - 2; i > -1; i--)	/* Deplacement des args */
	{
	  l[i + 1] = l[i];
	}
    }
  l[0] = (long) code - 150000;

  AddBufArg (l, size);
}

/* Ajout d'une instruction de test pour executer un ou plusieurs blocs */
/* enregistre l'instruction et le champs de ces blocs = NULL */
void
AddComBloc (int TypeCond, int NbLevelArg, int NbBloc)
{
  int i;
  int OldNA;
  int CurrInstr;

  /* Ajout de l'instruction de teste comme d'une commande */
  AddCom (TypeCond, NbLevelArg);

  /* On initialise ensuite les deux champs reserve à bloc1 et bloc2 */
  CurrInstr = PileBloc[TopPileB]->NbInstr;
  /* Attention NbArg peur changer si on utilise en arg une fonction */
  OldNA = PileBloc[TopPileB]->TabInstr[CurrInstr].NbArg;

  PileBloc[TopPileB]->TabInstr[CurrInstr].TabArg = (long *) realloc (
								      PileBloc[TopPileB]->TabInstr[CurrInstr].TabArg, sizeof (long) * (OldNA + NbBloc));
  for (i = 0; i < NbBloc; i++)
    {
      PileBloc[TopPileB]->TabInstr[CurrInstr].TabArg[OldNA + i] = 0;
    }
  PileBloc[TopPileB]->TabInstr[CurrInstr].NbArg = OldNA + NbBloc;
}

/* Creer un nouveau bloc, et l'empile: il devient le bloc courant */
void
EmpilerBloc ()
{
  Bloc *TmpBloc;

  TmpBloc = (Bloc *) calloc (1, sizeof (Bloc));
  TmpBloc->NbInstr = -1;
  TmpBloc->TabInstr = NULL;
  TopPileB++;
  PileBloc[TopPileB] = TmpBloc;

}

/* Depile le bloc d'initialisation du script et le range a sa place speciale */
void
DepilerBloc (int IdBloc)
{
  Bloc *Bloc1;
  Instr *IfInstr;

  Bloc1 = PileBloc[TopPileB];
  TopPileB--;
  IfInstr = &PileBloc[TopPileB]->TabInstr[PileBloc[TopPileB]->NbInstr];
  IfInstr->TabArg[IfInstr->NbArg - IdBloc] = (long) Bloc1;
}

/* Gestion des erreurs de syntaxes */
int
yyerror (char *errmsg)
{
  fprintf (stderr, "Line %d: %s\n", numligne, errmsg);
  return 0;
}



#line 346 "Compiler/bisonin"
typedef union
{
  char *str;
  int number;
}
YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		454
#define	YYFLAG		-32768
#define	YYNTBASE	86

#define YYTRANSLATE(x) ((unsigned)(x) <= 340 ? yytranslate[x] : 148)

static const char yytranslate[] =
{0,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 1, 2, 3, 4, 5,
 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
 36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
 46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
 56, 57, 58, 59, 60, 61, 62, 63, 64, 65,
 66, 67, 68, 69, 70, 71, 72, 73, 74, 75,
 76, 77, 78, 79, 80, 81, 82, 83, 84, 85
};

#if YYDEBUG != 0
static const short yyprhs[] =
{0,
 0, 6, 7, 8, 12, 16, 21, 26, 30, 34,
 38, 42, 46, 47, 53, 54, 60, 61, 69, 71,
 72, 76, 81, 86, 90, 94, 98, 102, 106, 110,
 114, 118, 122, 126, 130, 134, 135, 138, 141, 144,
 145, 147, 153, 154, 155, 160, 165, 167, 169, 171,
 175, 176, 180, 184, 188, 192, 196, 200, 204, 208,
 212, 216, 220, 224, 228, 232, 236, 240, 244, 248,
 252, 256, 260, 264, 267, 270, 273, 276, 279, 282,
 285, 288, 291, 294, 297, 300, 303, 306, 309, 312,
 315, 318, 321, 324, 327, 330, 333, 336, 341, 346,
 351, 358, 365, 370, 375, 380, 385, 390, 396, 401,
 402, 405, 410, 415, 420, 424, 428, 436, 437, 441,
 442, 446, 448, 452, 454, 464, 472, 474, 476, 478,
 480, 482, 484, 485, 488, 491, 496, 501, 506, 510,
 513, 517, 521, 525, 530, 533, 535, 538, 542, 544,
 547, 548, 551, 554, 557, 560, 563, 566, 572, 574,
 576, 578, 580, 582, 584, 589, 591, 593, 595, 597,
 602, 604, 606, 611, 613, 615, 620, 622, 624, 626,
 628, 630, 632
};

static const short yyrhs[] =
{87,
 88, 89, 90, 91, 0, 0, 0, 7, 4, 88,
 0, 29, 3, 88, 0, 9, 6, 6, 88, 0,
 8, 6, 6, 88, 0, 12, 4, 88, 0, 11,
 4, 88, 0, 13, 4, 88, 0, 14, 4, 88,
 0, 10, 3, 88, 0, 0, 16, 128, 39, 102,
 19, 0, 0, 17, 128, 39, 102, 19, 0, 0,
 15, 92, 20, 93, 95, 96, 91, 0, 6, 0,
 0, 21, 3, 93, 0, 22, 6, 6, 93, 0,
 23, 6, 6, 93, 0, 24, 6, 93, 0, 25,
 6, 93, 0, 26, 6, 93, 0, 27, 4, 93,
 0, 28, 4, 93, 0, 29, 3, 93, 0, 12,
 4, 93, 0, 11, 4, 93, 0, 13, 4, 93,
 0, 14, 4, 93, 0, 10, 3, 93, 0, 30,
 94, 93, 0, 0, 33, 94, 0, 35, 94, 0,
 34, 94, 0, 0, 19, 0, 18, 97, 36, 98,
 19, 0, 0, 0, 99, 40, 101, 98, 0, 100,
 40, 101, 98, 0, 37, 0, 38, 0, 6, 0,
 39, 102, 19, 0, 0, 41, 104, 102, 0, 31,
 120, 102, 0, 32, 122, 102, 0, 42, 105, 102,
 0, 43, 106, 102, 0, 47, 107, 102, 0, 48,
 108, 102, 0, 49, 109, 102, 0, 23, 110, 102,
 0, 22, 111, 102, 0, 27, 113, 102, 0, 29,
 112, 102, 0, 10, 114, 102, 0, 44, 115, 102,
 0, 45, 116, 102, 0, 66, 117, 102, 0, 67,
 118, 102, 0, 60, 119, 102, 0, 63, 121, 102,
 0, 71, 123, 102, 0, 74, 124, 102, 0, 77,
 125, 102, 0, 41, 104, 0, 31, 120, 0, 32,
 122, 0, 42, 105, 0, 43, 106, 0, 47, 107,
 0, 48, 108, 0, 49, 109, 0, 23, 110, 0,
 22, 111, 0, 27, 113, 0, 29, 112, 0, 10,
 114, 0, 44, 115, 0, 45, 116, 0, 66, 117,
 0, 67, 118, 0, 60, 119, 0, 63, 121, 0,
 74, 124, 0, 77, 125, 0, 139, 141, 0, 139,
 143, 0, 139, 143, 0, 139, 143, 139, 143, 0,
 139, 143, 139, 143, 0, 139, 143, 139, 143, 0,
 139, 143, 139, 143, 139, 143, 0, 139, 143, 139,
 143, 139, 143, 0, 139, 143, 139, 144, 0, 139,
 143, 139, 145, 0, 139, 143, 139, 144, 0, 139,
 143, 139, 145, 0, 139, 143, 139, 145, 0, 139,
 146, 65, 139, 141, 0, 139, 143, 139, 143, 0,
 0, 139, 143, 0, 139, 143, 139, 141, 0, 139,
 144, 139, 141, 0, 126, 128, 129, 127, 0, 131,
 128, 130, 0, 132, 128, 130, 0, 139, 142, 139,
 147, 139, 142, 72, 0, 0, 73, 128, 130, 0,
 0, 39, 102, 19, 0, 103, 0, 39, 102, 19,
 0, 103, 0, 139, 146, 65, 139, 142, 75, 139,
 142, 76, 0, 139, 142, 139, 147, 139, 142, 76,
 0, 5, 0, 3, 0, 4, 0, 6, 0, 37,
 0, 38, 0, 0, 46, 143, 0, 53, 143, 0,
 54, 145, 143, 143, 0, 55, 145, 145, 143, 0,
 56, 145, 145, 143, 0, 58, 143, 143, 0, 59,
 145, 0, 50, 143, 143, 0, 52, 143, 143, 0,
 51, 143, 143, 0, 57, 145, 143, 143, 0, 61,
 145, 0, 62, 0, 64, 143, 0, 68, 143, 143,
 0, 69, 0, 70, 143, 0, 0, 137, 141, 0,
 138, 141, 0, 133, 141, 0, 135, 141, 0, 134,
 141, 0, 136, 141, 0, 78, 139, 140, 79, 141,
 0, 133, 0, 137, 0, 138, 0, 135, 0, 134,
 0, 136, 0, 78, 139, 140, 79, 0, 137, 0,
 138, 0, 136, 0, 133, 0, 78, 139, 140, 79,
 0, 133, 0, 134, 0, 78, 139, 140, 79, 0,
 133, 0, 135, 0, 78, 139, 140, 79, 0, 133,
 0, 83, 0, 81, 0, 80, 0, 82, 0, 84,
 0, 85, 0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] =
{0,
 369, 372, 376, 377, 380, 383, 388, 393, 396, 399,
 402, 405, 411, 412, 417, 418, 427, 428, 431, 446,
 447, 451, 455, 460, 463, 466, 469, 472, 475, 478,
 481, 484, 487, 490, 493, 495, 496, 499, 502, 508,
 519, 520, 523, 525, 526, 527, 530, 531, 534, 537,
 542, 543, 544, 545, 546, 547, 548, 549, 550, 551,
 552, 553, 554, 555, 556, 557, 558, 559, 560, 561,
 562, 563, 564, 568, 569, 570, 571, 572, 573, 574,
 575, 576, 577, 578, 579, 580, 581, 582, 583, 584,
 585, 586, 587, 588, 591, 593, 595, 597, 599, 601,
 603, 605, 607, 609, 611, 613, 615, 617, 619, 621,
 623, 625, 627, 629, 631, 633, 637, 639, 640, 642,
 644, 645, 648, 649, 653, 657, 662, 664, 666, 668,
 670, 672, 674, 676, 677, 678, 679, 680, 681, 682,
 683, 684, 685, 686, 687, 688, 689, 690, 691, 692,
 697, 698, 699, 700, 701, 702, 703, 704, 709, 710,
 711, 712, 713, 714, 715, 719, 720, 721, 722, 723,
 727, 728, 729, 733, 734, 735, 739, 743, 744, 745,
 746, 747, 748
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char *const yytname[] =
{"$", "error", "$undefined.", "STR", "GSTR",
 "VAR", "NUMBER", "WINDOWTITLE", "WINDOWSIZE", "WINDOWPOSITION", "FONT", "FORECOLOR",
 "BACKCOLOR", "SHADCOLOR", "LICOLOR", "OBJECT", "INIT", "PERIODICTASK", "MAIN", "END",
 "PROP", "TYPE", "SIZE", "POSITION", "VALUE", "VALUEMIN", "VALUEMAX", "TITLE", "SWALLOWEXEC",
 "ICON", "FLAGS", "WARP", "WRITETOFILE", "HIDDEN", "CANBESELECTED", "NORELIEFSTRING",
 "CASE", "SINGLECLIC", "DOUBLECLIC", "BEG", "POINT", "EXEC", "HIDE", "SHOW", "CHFORECOLOR",
 "CHBACKCOLOR", "GETVALUE", "CHVALUE", "CHVALUEMAX", "CHVALUEMIN", "ADD", "DIV", "MULT",
 "GETTITLE", "GETOUTPUT", "GETASOPTION", "SETASOPTION", "STRCOPY", "NUMTOHEX", "HEXTONUM",
 "QUIT", "LAUNCHSCRIPT", "GETSCRIPTFATHER", "SENDTOSCRIPT", "RECEIVFROMSCRIPT", "GET",
 "SET", "SENDSIGN", "REMAINDEROFDIV", "GETTIME", "GETSCRIPTARG", "IF", "THEN", "ELSE",
 "FOR", "TO", "DO", "WHILE", "BEGF", "ENDF", "EQUAL", "INFEQ", "SUPEQ", "INF", "SUP", "DIFF",
 "script", "initvar", "head", "initbloc", "periodictask", "object", "id", "init", "flags",
 "verify", "mainloop", "addtabcase", "case", "clic", "number", "bloc", "instr", "oneinstr",
 "exec", "hide", "show", "chvalue", "chvaluemax", "chvaluemin", "position", "size", "icon",
 "title", "font", "chforecolor", "chbackcolor", "set", "sendsign", "quit", "warp", "sendtoscript",
 "writetofile", "ifthenelse", "loop", "while", "headif", "else", "creerbloc", "bloc1",
 "bloc2", "headloop", "headwhile", "var", "str", "gstr", "num", "singleclic2", "doubleclic2",
 "addlbuff", "function", "args", "arg", "numarg", "strarg", "gstrarg", "vararg", "compare", NULL
};
#endif

static const short yyr1[] =
{0,
 86, 87, 88, 88, 88, 88, 88, 88, 88, 88,
 88, 88, 89, 89, 90, 90, 91, 91, 92, 93,
 93, 93, 93, 93, 93, 93, 93, 93, 93, 93,
 93, 93, 93, 93, 93, 94, 94, 94, 94, 95,
 96, 96, 97, 98, 98, 98, 99, 99, 100, 101,
 102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
 102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
 102, 102, 102, 103, 103, 103, 103, 103, 103, 103,
 103, 103, 103, 103, 103, 103, 103, 103, 103, 103,
 103, 103, 103, 103, 104, 105, 106, 107, 108, 109,
 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
 120, 121, 122, 123, 124, 125, 126, 127, 127, 128,
 129, 129, 130, 130, 131, 132, 133, 134, 135, 136,
 137, 138, 139, 140, 140, 140, 140, 140, 140, 140,
 140, 140, 140, 140, 140, 140, 140, 140, 140, 140,
 141, 141, 141, 141, 141, 141, 141, 141, 142, 142,
 142, 142, 142, 142, 142, 143, 143, 143, 143, 143,
 144, 144, 144, 145, 145, 145, 146, 147, 147, 147,
 147, 147, 147
};

static const short yyr2[] =
{0,
 5, 0, 0, 3, 3, 4, 4, 3, 3, 3,
 3, 3, 0, 5, 0, 5, 0, 7, 1, 0,
 3, 4, 4, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 0, 2, 2, 2, 0,
 1, 5, 0, 0, 4, 4, 1, 1, 1, 3,
 0, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 4, 4, 4,
 6, 6, 4, 4, 4, 4, 4, 5, 4, 0,
 2, 4, 4, 4, 3, 3, 7, 0, 3, 0,
 3, 1, 3, 1, 9, 7, 1, 1, 1, 1,
 1, 1, 0, 2, 2, 4, 4, 4, 3, 2,
 3, 3, 3, 4, 2, 1, 2, 3, 1, 2,
 0, 2, 2, 2, 2, 2, 2, 5, 1, 1,
 1, 1, 1, 1, 4, 1, 1, 1, 1, 4,
 1, 1, 4, 1, 1, 4, 1, 1, 1, 1,
 1, 1, 1
};

static const short yydefact[] =
{2,
 3, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 13, 3, 0, 0, 3, 3, 3, 3, 3, 3,
 120, 15, 4, 3, 3, 12, 9, 8, 10, 11,
 5, 0, 120, 17, 7, 6, 51, 0, 0, 1,
 133, 133, 133, 133, 133, 133, 133, 133, 133, 133,
 133, 133, 133, 133, 133, 110, 133, 133, 133, 133,
 133, 133, 0, 51, 19, 0, 51, 0, 51, 0,
 51, 0, 51, 0, 51, 0, 51, 0, 51, 0,
 51, 151, 51, 0, 51, 0, 51, 0, 51, 0,
 51, 0, 51, 0, 51, 0, 51, 51, 0, 51,
 0, 51, 0, 51, 120, 0, 51, 120, 0, 51,
 120, 0, 14, 0, 20, 64, 127, 130, 131, 132,
 133, 169, 168, 166, 167, 133, 61, 133, 60, 133,
 62, 133, 63, 133, 53, 111, 54, 128, 133, 171,
 172, 133, 52, 129, 133, 151, 151, 151, 151, 151,
 151, 95, 55, 96, 56, 97, 65, 133, 66, 133,
 57, 133, 58, 133, 59, 133, 69, 70, 133, 67,
 177, 0, 68, 133, 71, 0, 133, 159, 163, 162,
 164, 160, 161, 133, 72, 0, 0, 73, 0, 133,
 16, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 36, 40, 0, 0, 0,
 0, 0, 0, 0, 151, 0, 154, 156, 155, 157,
 152, 153, 0, 0, 0, 0, 0, 151, 133, 0,
 133, 133, 133, 133, 133, 133, 133, 51, 133, 133,
 133, 133, 133, 133, 133, 133, 110, 133, 133, 133,
 133, 133, 122, 118, 0, 0, 51, 124, 115, 133,
 116, 0, 20, 20, 20, 20, 20, 20, 0, 0,
 20, 20, 20, 20, 20, 20, 36, 36, 36, 20,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 146, 0, 0, 149, 0, 0, 105,
 133, 133, 133, 174, 175, 104, 103, 0, 113, 0,
 106, 107, 98, 99, 100, 112, 151, 109, 86, 83,
 82, 84, 85, 75, 76, 0, 74, 77, 78, 87,
 88, 79, 80, 81, 91, 92, 89, 90, 93, 94,
 120, 114, 0, 180, 179, 181, 178, 182, 183, 133,
 0, 0, 133, 34, 31, 30, 32, 33, 21, 20,
 20, 24, 25, 26, 27, 28, 29, 37, 39, 38,
 35, 43, 41, 17, 134, 0, 0, 0, 135, 0,
 0, 0, 0, 0, 140, 145, 147, 0, 150, 170,
 0, 0, 0, 173, 151, 108, 121, 0, 165, 0,
 123, 0, 0, 22, 23, 0, 18, 141, 143, 142,
 0, 0, 0, 0, 139, 148, 102, 101, 0, 158,
 119, 0, 133, 0, 44, 136, 137, 138, 144, 176,
 117, 0, 126, 49, 47, 48, 0, 0, 0, 0,
 42, 0, 0, 125, 51, 44, 44, 0, 45, 46,
 50, 0, 0, 0
};

static const short yydefgoto[] =
{452,
 1, 11, 22, 34, 40, 66, 207, 280, 281, 374,
 406, 437, 438, 439, 446, 63, 258, 81, 83, 85,
 91, 93, 95, 71, 69, 75, 73, 67, 87, 89,
 100, 102, 97, 77, 98, 79, 104, 107, 110, 105,
 342, 32, 254, 259, 108, 111, 122, 147, 148, 123,
 124, 125, 68, 299, 152, 184, 126, 142, 306, 172,
 350
};

static const short yypact[] =
{-32768,
 275, 37, 39, 43, 26, 47, 51, 53, 55, 36,
 27, 275, 57, 60, 275, 275, 275, 275, 275, 275,
 -32768, 44, -32768, 275, 275, -32768, -32768, -32768, -32768, -32768,
 -32768, 29, -32768, 70, -32768, -32768, 560, 31, 69, -32768,
 -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768,
 -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768,
 -32768, -32768, 68, 560, -32768, 71, 560, 151, 560, 151,
 560, 151, 560, 151, 560, 151, 560, 151, 560, 20,
 560, 77, 560, 151, 560, 151, 560, 151, 560, 151,
 560, 151, 560, 151, 560, 151, 560, 560, 151, 560,
 84, 560, 151, 560, -32768, 261, 560, -32768, 84, 560,
 -32768, 261, -32768, 74, 462, -32768, -32768, -32768, -32768, -32768,
 -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768,
 -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768,
 -32768, -32768, -32768, -32768, -32768, 77, 77, 77, 77, 77,
 77, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768,
 -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768,
 -32768, 41, -32768, -32768, -32768, 666, -32768, -32768, -32768, -32768,
 -32768, -32768, -32768, -32768, -32768, 715, 56, -32768, 715, -32768,
 -32768, 91, 100, 104, 119, 129, 133, 132, 134, 137,
 138, 140, 145, 147, 149, 78, -32768, 589, 20, 151,
 151, 23, 20, 589, 77, 589, -32768, -32768, -32768, -32768,
 -32768, -32768, 23, 23, 151, 151, 151, 77, -32768, 151,
 -32768, -32768, -32768, -32768, -32768, -32768, -32768, 560, -32768, -32768,
 -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768,
 -32768, -32768, -32768, 80, 589, 85, 560, -32768, -32768, -32768,
 -32768, 85, 462, 462, 462, 462, 462, 462, 152, 171,
 462, 462, 462, 462, 462, 462, 78, 78, 78, 462,
 14, 151, 151, 151, 151, 151, 23, 23, 23, 23,
 151, 23, 23, -32768, 151, 151, -32768, 151, 99, -32768,
 -32768, -32768, -32768, -32768, -32768, -32768, -32768, 101, -32768, 117,
 -32768, -32768, -32768, -32768, -32768, -32768, 77, -32768, -32768, -32768,
 -32768, -32768, -32768, -32768, -32768, 160, -32768, -32768, -32768, -32768,
 -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768,
 -32768, -32768, 136, -32768, -32768, -32768, -32768, -32768, -32768, -32768,
 192, 261, -32768, -32768, -32768, -32768, -32768, -32768, -32768, 462,
 462, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768,
 -32768, -32768, -32768, 70, -32768, 151, 151, 151, -32768, 151,
 23, 23, 151, 151, -32768, -32768, -32768, 151, -32768, -32768,
 151, 151, 589, -32768, 77, -32768, -32768, 715, -32768, 261,
 -32768, 141, 261, -32768, -32768, 177, -32768, -32768, -32768, -32768,
 151, 151, 151, 151, -32768, -32768, -32768, -32768, 142, -32768,
 -32768, 148, -32768, 143, 72, -32768, -32768, -32768, -32768, -32768,
 -32768, 261, -32768, -32768, -32768, -32768, 198, 182, 183, 150,
 -32768, 186, 186, -32768, 560, 72, 72, 208, -32768, -32768,
 -32768, 230, 232, -32768
};

static const short yypgoto[] =
{-32768,
 -32768, 487, -32768, -32768, -141, -32768, 188, -149, -32768, -32768,
 -32768, -374, -32768, -32768, -208, -33, 63, 4, 6, 3,
 7, 2, 9, 19, 30, 40, 24, 59, 49, 52,
 45, 18, 50, 64, 48, 66, -32768, 58, 54, -32768,
 -32768, -9, -32768, -167, -32768, -32768, 25, -59, 33, 13,
 35, 130, -42, -179, 123, -98, 290, -183, 112, 201,
 62
};


#define	YYLAST		792


static const short yytable[] =
{70,
 72, 74, 76, 78, 80, 82, 84, 86, 88, 90,
 92, 94, 96, 190, 99, 101, 103, 106, 109, 112,
 141, 261, 138, 38, 117, 300, 144, 117, 15, 307,
 114, 372, 373, 116, 308, 127, 310, 129, 20, 131,
 12, 133, 21, 135, 13, 137, 179, 143, 14, 153,
 16, 155, 179, 157, 17, 159, 18, 161, 19, 163,
 33, 165, 24, 167, 168, 25, 170, 37, 173, 64,
 175, 449, 450, 185, 65, 343, 188, 434, 208, 138,
 144, 117, 118, 209, 39, 210, 113, 211, 117, 212,
 115, 213, 191, 263, 149, 176, 214, 139, 186, 215,
 303, 189, 216, 264, 140, 229, 146, 265, 435, 436,
 277, 278, 279, 119, 120, 223, 150, 224, 181, 225,
 260, 226, 266, 227, 181, 171, 228, 368, 369, 370,
 178, 230, 267, 171, 255, 268, 178, 269, 180, 270,
 182, 256, 271, 272, 180, 273, 182, 262, 274, 141,
 275, 276, 341, 141, 145, 117, 118, 360, 149, 149,
 149, 149, 149, 149, 344, 345, 346, 347, 348, 349,
 146, 146, 146, 146, 146, 146, 361, 390, 397, 394,
 150, 150, 150, 150, 150, 150, 317, 119, 120, 70,
 72, 74, 76, 78, 80, 395, 82, 84, 86, 88,
 90, 92, 94, 96, 326, 99, 101, 103, 109, 112,
 401, 151, 425, 419, 399, 423, 441, 352, 433, 431,
 430, 442, 443, 351, 445, 444, 451, 149, 121, 453,
 421, 454, 407, 140, 447, 183, 304, 140, 253, 146,
 149, 183, 327, 329, 305, 328, 333, 304, 304, 150,
 332, 321, 146, 402, 334, 305, 305, 322, 391, 392,
 393, 320, 150, 138, 144, 117, 118, 338, 217, 218,
 219, 220, 221, 222, 323, 151, 151, 151, 151, 151,
 151, 2, 3, 4, 5, 6, 7, 8, 9, 319,
 330, 0, 179, 337, 331, 336, 335, 119, 120, 324,
 0, 422, 325, 10, 424, 340, 0, 400, 339, 187,
 403, 304, 304, 304, 304, 0, 304, 304, 0, 305,
 305, 305, 305, 353, 305, 305, 0, 0, 0, 149,
 0, 398, 0, 440, 311, 312, 0, 309, 177, 0,
 179, 146, 0, 179, 151, 0, 0, 0, 0, 0,
 316, 150, 0, 0, 0, 0, 0, 151, 0, 128,
 0, 130, 0, 132, 181, 134, 0, 136, 0, 0,
 0, 0, 179, 154, 0, 156, 178, 158, 0, 160,
 432, 162, 0, 164, 180, 166, 182, 0, 169, 0,
 0, 0, 174, 0, 0, 0, 0, 0, 380, 381,
 382, 383, 0, 385, 386, 304, 304, 149, 0, 0,
 0, 448, 181, 305, 305, 181, 0, 0, 0, 146,
 0, 0, 0, 0, 178, 0, 0, 178, 0, 150,
 0, 0, 180, 0, 182, 180, 0, 182, 0, 396,
 0, 0, 0, 0, 181, 0, 151, 0, 0, 0,
 354, 355, 356, 357, 358, 359, 178, 0, 362, 363,
 364, 365, 366, 367, 180, 0, 182, 371, 0, 0,
 0, 192, 193, 194, 195, 196, 0, 0, 0, 0,
 0, 183, 197, 198, 199, 200, 201, 202, 203, 204,
 205, 206, 412, 413, 0, 0, 0, 0, 23, 301,
 302, 26, 27, 28, 29, 30, 31, 0, 0, 0,
 35, 36, 0, 0, 313, 314, 315, 420, 0, 318,
 0, 0, 0, 0, 151, 0, 0, 0, 0, 183,
 0, 0, 183, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 404, 405, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 183, 0, 0, 0, 0, 0, 0, 0, 41,
 0, 375, 376, 377, 378, 379, 0, 0, 0, 0,
 384, 42, 43, 0, 387, 388, 44, 389, 45, 0,
 46, 47, 0, 0, 0, 0, 0, 0, 0, 0,
 48, 49, 50, 51, 52, 0, 53, 54, 55, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 56,
 0, 0, 57, 0, 0, 58, 59, 0, 0, 0,
 60, 0, 0, 61, 282, 0, 62, 0, 283, 284,
 285, 286, 287, 288, 289, 290, 291, 292, 0, 293,
 294, 0, 295, 0, 0, 0, 296, 297, 298, 0,
 0, 0, 0, 0, 0, 408, 409, 410, 0, 411,
 0, 0, 414, 415, 0, 231, 0, 416, 0, 0,
 417, 418, 0, 0, 0, 0, 0, 232, 233, 0,
 0, 0, 234, 0, 235, 0, 236, 237, 0, 0,
 426, 427, 428, 429, 238, 0, 239, 240, 241, 242,
 243, 0, 244, 245, 246, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 231, 247, 0, 0, 248, 0,
 0, 249, 250, 0, 0, 0, 232, 233, 0, 251,
 0, 234, 252, 235, 0, 236, 237, 0, 0, 0,
 0, 0, 0, 257, 0, 239, 240, 241, 242, 243,
 0, 244, 245, 246, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 247, 0, 0, 248, 0, 0,
 249, 250, 0, 0, 0, 0, 0, 0, 251, 0,
 0, 252
};

static const short yycheck[] =
{42,
 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,
 53, 54, 55, 112, 57, 58, 59, 60, 61, 62,
 80, 189, 3, 33, 5, 209, 4, 5, 3, 213,
 64, 18, 19, 67, 214, 69, 216, 71, 3, 73,
 4, 75, 16, 77, 6, 79, 106, 81, 6, 83,
 4, 85, 112, 87, 4, 89, 4, 91, 4, 93,
 17, 95, 6, 97, 98, 6, 100, 39, 102, 39,
 104, 446, 447, 107, 6, 255, 110, 6, 121, 3,
 4, 5, 6, 126, 15, 128, 19, 130, 5, 132,
 20, 134, 19, 3, 82, 105, 139, 78, 108, 142,
 78, 111, 145, 4, 80, 65, 82, 4, 37, 38,
 33, 34, 35, 37, 38, 158, 82, 160, 106, 162,
 65, 164, 4, 166, 112, 101, 169, 277, 278, 279,
 106, 174, 4, 109, 177, 3, 112, 6, 106, 6,
 106, 184, 6, 6, 112, 6, 112, 190, 4, 209,
 4, 3, 73, 213, 78, 5, 6, 6, 146, 147,
 148, 149, 150, 151, 80, 81, 82, 83, 84, 85,
 146, 147, 148, 149, 150, 151, 6, 79, 19, 79,
 146, 147, 148, 149, 150, 151, 229, 37, 38, 232,
 233, 234, 235, 236, 237, 79, 239, 240, 241, 242,
 243, 244, 245, 246, 238, 248, 249, 250, 251, 252,
 19, 82, 36, 393, 79, 75, 19, 260, 76, 72,
 79, 40, 40, 257, 39, 76, 19, 215, 78, 0,
 398, 0, 374, 209, 443, 106, 212, 213, 176, 215,
 228, 112, 239, 241, 212, 240, 245, 223, 224, 215,
 244, 233, 228, 352, 246, 223, 224, 234, 301, 302,
 303, 232, 228, 3, 4, 5, 6, 250, 146, 147,
 148, 149, 150, 151, 235, 146, 147, 148, 149, 150,
 151, 7, 8, 9, 10, 11, 12, 13, 14, 231,
 242, -1, 352, 249, 243, 248, 247, 37, 38, 236,
 -1, 400, 237, 29, 403, 252, -1, 350, 251, 109,
 353, 287, 288, 289, 290, -1, 292, 293, -1, 287,
 288, 289, 290, 262, 292, 293, -1, -1, -1, 317,
 -1, 341, -1, 432, 223, 224, -1, 215, 78, -1,
 400, 317, -1, 403, 215, -1, -1, -1, -1, -1,
 228, 317, -1, -1, -1, -1, -1, 228, -1, 70,
 -1, 72, -1, 74, 352, 76, -1, 78, -1, -1,
 -1, -1, 432, 84, -1, 86, 352, 88, -1, 90,
 423, 92, -1, 94, 352, 96, 352, -1, 99, -1,
 -1, -1, 103, -1, -1, -1, -1, -1, 287, 288,
 289, 290, -1, 292, 293, 381, 382, 395, -1, -1,
 -1, 445, 400, 381, 382, 403, -1, -1, -1, 395,
 -1, -1, -1, -1, 400, -1, -1, 403, -1, 395,
 -1, -1, 400, -1, 400, 403, -1, 403, -1, 317,
 -1, -1, -1, -1, 432, -1, 317, -1, -1, -1,
 263, 264, 265, 266, 267, 268, 432, -1, 271, 272,
 273, 274, 275, 276, 432, -1, 432, 280, -1, -1,
 -1, 10, 11, 12, 13, 14, -1, -1, -1, -1,
 -1, 352, 21, 22, 23, 24, 25, 26, 27, 28,
 29, 30, 381, 382, -1, -1, -1, -1, 12, 210,
 211, 15, 16, 17, 18, 19, 20, -1, -1, -1,
 24, 25, -1, -1, 225, 226, 227, 395, -1, 230,
 -1, -1, -1, -1, 395, -1, -1, -1, -1, 400,
 -1, -1, 403, -1, -1, -1, -1, -1, -1, -1,
 -1, -1, -1, -1, -1, -1, -1, 360, 361, -1,
 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
 -1, 432, -1, -1, -1, -1, -1, -1, -1, 10,
 -1, 282, 283, 284, 285, 286, -1, -1, -1, -1,
 291, 22, 23, -1, 295, 296, 27, 298, 29, -1,
 31, 32, -1, -1, -1, -1, -1, -1, -1, -1,
 41, 42, 43, 44, 45, -1, 47, 48, 49, -1,
 -1, -1, -1, -1, -1, -1, -1, -1, -1, 60,
 -1, -1, 63, -1, -1, 66, 67, -1, -1, -1,
 71, -1, -1, 74, 46, -1, 77, -1, 50, 51,
 52, 53, 54, 55, 56, 57, 58, 59, -1, 61,
 62, -1, 64, -1, -1, -1, 68, 69, 70, -1,
 -1, -1, -1, -1, -1, 376, 377, 378, -1, 380,
 -1, -1, 383, 384, -1, 10, -1, 388, -1, -1,
 391, 392, -1, -1, -1, -1, -1, 22, 23, -1,
 -1, -1, 27, -1, 29, -1, 31, 32, -1, -1,
 411, 412, 413, 414, 39, -1, 41, 42, 43, 44,
 45, -1, 47, 48, 49, -1, -1, -1, -1, -1,
 -1, -1, -1, -1, 10, 60, -1, -1, 63, -1,
 -1, 66, 67, -1, -1, -1, 22, 23, -1, 74,
 -1, 27, 77, 29, -1, 31, 32, -1, -1, -1,
 -1, -1, -1, 39, -1, 41, 42, 43, 44, 45,
 -1, 47, 48, 49, -1, -1, -1, -1, -1, -1,
 -1, -1, -1, -1, 60, -1, -1, 63, -1, -1,
 66, 67, -1, -1, -1, -1, -1, -1, 74, -1,
 -1, 77
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/share/bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi)
#include <alloca.h>
#else /* not sparc */
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
#pragma alloca
#else /* not MSDOS, __TURBOC__, or _AIX */
#ifdef __hpux
#ifdef __cplusplus
extern "C"
{
  void *alloca (unsigned int);
};
#else /* not __cplusplus */
void *alloca ();
#endif /* not __cplusplus */
#endif /* __hpux */
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */

/* This is the parser code that is written into each bison parser
   when the %semantic_parser declaration is not specified in the grammar.
   It was written by Richard Stallman by simplifying the hairy parser
   used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	return(0)
#define YYABORT 	return(1)
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int yychar;			/*  the lookahead symbol                */
YYSTYPE yylval;			/*  the semantic value of the           */
				/*  lookahead symbol                    */

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead     */
				/*  symbol                              */
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace     */
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks       */

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
   (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
int yyparse (void);
#endif

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else /* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, int count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 196 "/usr/share/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

int
yyparse (YYPARSE_PARAM_ARG)
YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;		/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short yyssa[YYINITDEPTH];	/*  the state stack                     */
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack            */

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack                  */
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return         */
  /*  semantic values from the action     */
  /*  routines                            */

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf (stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
         the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
         but that might be undefined if yyoverflow is a macro.  */
      yyoverflow ("parser stack overflow",
		  &yyss1, size * sizeof (*yyssp),
		  &yyvs1, size * sizeof (*yyvsp),
		  &yyls1, size * sizeof (*yylsp),
		  &yystacksize);
#else
      yyoverflow ("parser stack overflow",
		  &yyss1, size * sizeof (*yyssp),
		  &yyvs1, size * sizeof (*yyvsp),
		  &yystacksize);
#endif

      yyss = yyss1;
      yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror ("parser stack overflow");
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *) yyss, (char *) yyss1, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *) yyvs, (char *) yyvs1, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) alloca (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *) yyls, (char *) yyls1, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf (stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf (stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf (stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf (stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE (yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
     New state is final state => don't bother to shift,
     just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf (stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1 - yylen];	/* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn)
    {

    case 2:
#line 372 "Compiler/bisonin"
      {
	InitVarGlob ();;
	break;
      }
    case 4:
#line 377 "Compiler/bisonin"
      {				/* Titre de la fenetre */
	scriptprop->titlewin = yyvsp[-1].str;
	;
	break;
      }
    case 5:
#line 380 "Compiler/bisonin"
      {
	scriptprop->icon = yyvsp[-1].str;
	;
	break;
      }
    case 6:
#line 384 "Compiler/bisonin"
      {				/* Position et taille de la fenetre */
	scriptprop->x = yyvsp[-2].number;
	scriptprop->y = yyvsp[-1].number;
	;
	break;
      }
    case 7:
#line 389 "Compiler/bisonin"
      {				/* Position et taille de la fenetre */
	scriptprop->width = yyvsp[-2].number;
	scriptprop->height = yyvsp[-1].number;
	;
	break;
      }
    case 8:
#line 393 "Compiler/bisonin"
      {				/* Couleur de fond */
	scriptprop->backcolor = yyvsp[-1].str;
	;
	break;
      }
    case 9:
#line 396 "Compiler/bisonin"
      {				/* Couleur des lignes */
	scriptprop->forecolor = yyvsp[-1].str;
	;
	break;
      }
    case 10:
#line 399 "Compiler/bisonin"
      {				/* Couleur des lignes */
	scriptprop->shadcolor = yyvsp[-1].str;
	;
	break;
      }
    case 11:
#line 402 "Compiler/bisonin"
      {				/* Couleur des lignes */
	scriptprop->licolor = yyvsp[-1].str;
	;
	break;
      }
    case 12:
#line 405 "Compiler/bisonin"
      {
	scriptprop->font = yyvsp[-1].str;
	;
	break;
      }
    case 14:
#line 412 "Compiler/bisonin"
      {
	scriptprop->initbloc = PileBloc[TopPileB];
	TopPileB--;
	;
	break;
      }
    case 16:
#line 418 "Compiler/bisonin"
      {
	scriptprop->periodictasks = PileBloc[TopPileB];
	TopPileB--;
	;
	break;
      }
    case 19:
#line 431 "Compiler/bisonin"
      {
	nbobj++;
	if (nbobj > 100)
	  {
	    yyerror ("Too many items\n");
	    exit (1);
	  }
	if ((yyvsp[0].number < 1) || (yyvsp[0].number > 100))
	  {
	    yyerror ("Choose item id between 1 and 100\n");
	    exit (1);
	  }
	if (TabIdObj[yyvsp[0].number] != -1)
	  {
	    i = yyvsp[0].number;
	    fprintf (stderr, "Line %d: item id %d already used:\n", numligne, yyvsp[0].number);
	    exit (1);
	  }
	TabIdObj[yyvsp[0].number] = nbobj;
	(*tabobj)[nbobj].id = yyvsp[0].number;
	;
	break;
      }
    case 21:
#line 447 "Compiler/bisonin"
      {
	(*tabobj)[nbobj].type = yyvsp[-1].str;
	HasType = 1;
	;
	break;
      }
    case 22:
#line 451 "Compiler/bisonin"
      {
	(*tabobj)[nbobj].width = yyvsp[-2].number;
	(*tabobj)[nbobj].height = yyvsp[-1].number;
	;
	break;
      }
    case 23:
#line 455 "Compiler/bisonin"
      {
	(*tabobj)[nbobj].x = yyvsp[-2].number;
	(*tabobj)[nbobj].y = yyvsp[-1].number;
	HasPosition = 1;
	;
	break;
      }
    case 24:
#line 460 "Compiler/bisonin"
      {
	(*tabobj)[nbobj].value = yyvsp[-1].number;
	;
	break;
      }
    case 25:
#line 463 "Compiler/bisonin"
      {
	(*tabobj)[nbobj].value2 = yyvsp[-1].number;
	;
	break;
      }
    case 26:
#line 466 "Compiler/bisonin"
      {
	(*tabobj)[nbobj].value3 = yyvsp[-1].number;
	;
	break;
      }
    case 27:
#line 469 "Compiler/bisonin"
      {
	(*tabobj)[nbobj].title = yyvsp[-1].str;
	;
	break;
      }
    case 28:
#line 472 "Compiler/bisonin"
      {
	(*tabobj)[nbobj].swallow = yyvsp[-1].str;
	;
	break;
      }
    case 29:
#line 475 "Compiler/bisonin"
      {
	(*tabobj)[nbobj].icon = yyvsp[-1].str;
	;
	break;
      }
    case 30:
#line 478 "Compiler/bisonin"
      {
	(*tabobj)[nbobj].backcolor = yyvsp[-1].str;
	;
	break;
      }
    case 31:
#line 481 "Compiler/bisonin"
      {
	(*tabobj)[nbobj].forecolor = yyvsp[-1].str;
	;
	break;
      }
    case 32:
#line 484 "Compiler/bisonin"
      {
	(*tabobj)[nbobj].shadcolor = yyvsp[-1].str;
	;
	break;
      }
    case 33:
#line 487 "Compiler/bisonin"
      {
	(*tabobj)[nbobj].licolor = yyvsp[-1].str;
	;
	break;
      }
    case 34:
#line 490 "Compiler/bisonin"
      {
	(*tabobj)[nbobj].font = yyvsp[-1].str;
	;
	break;
      }
    case 37:
#line 496 "Compiler/bisonin"
      {
	(*tabobj)[nbobj].flags[0] = True;
	;
	break;
      }
    case 38:
#line 499 "Compiler/bisonin"
      {
	(*tabobj)[nbobj].flags[1] = True;
	;
	break;
      }
    case 39:
#line 502 "Compiler/bisonin"
      {
	(*tabobj)[nbobj].flags[2] = True;
	;
	break;
      }
    case 40:
#line 508 "Compiler/bisonin"
      {
	if (!HasPosition)
	  {
	    yyerror ("No position for object");
	    exit (1);
	  }
	if (!HasType)
	  {
	    yyerror ("No type for object");
	    exit (1);
	  }
	HasPosition = 0;
	HasType = 0;
	;
	break;
      }
    case 41:
#line 519 "Compiler/bisonin"
      {
	InitObjTabCase (0);;
	break;
      }
    case 43:
#line 523 "Compiler/bisonin"
      {
	InitObjTabCase (1);;
	break;
      }
    case 47:
#line 530 "Compiler/bisonin"
      {
	InitCase (-1);;
	break;
      }
    case 48:
#line 531 "Compiler/bisonin"
      {
	InitCase (-2);;
	break;
      }
    case 49:
#line 534 "Compiler/bisonin"
      {
	InitCase (yyvsp[0].number);;
	break;
      }
    case 95:
#line 591 "Compiler/bisonin"
      {
	AddCom (1, 1);;
	break;
      }
    case 96:
#line 593 "Compiler/bisonin"
      {
	AddCom (2, 1);;
	break;
      }
    case 97:
#line 595 "Compiler/bisonin"
      {
	AddCom (3, 1);;
	break;
      }
    case 98:
#line 597 "Compiler/bisonin"
      {
	AddCom (4, 2);;
	break;
      }
    case 99:
#line 599 "Compiler/bisonin"
      {
	AddCom (21, 2);;
	break;
      }
    case 100:
#line 601 "Compiler/bisonin"
      {
	AddCom (22, 2);;
	break;
      }
    case 101:
#line 603 "Compiler/bisonin"
      {
	AddCom (5, 3);;
	break;
      }
    case 102:
#line 605 "Compiler/bisonin"
      {
	AddCom (6, 3);;
	break;
      }
    case 103:
#line 607 "Compiler/bisonin"
      {
	AddCom (7, 2);;
	break;
      }
    case 104:
#line 609 "Compiler/bisonin"
      {
	AddCom (8, 2);;
	break;
      }
    case 105:
#line 611 "Compiler/bisonin"
      {
	AddCom (9, 2);;
	break;
      }
    case 106:
#line 613 "Compiler/bisonin"
      {
	AddCom (10, 2);;
	break;
      }
    case 107:
#line 615 "Compiler/bisonin"
      {
	AddCom (19, 2);;
	break;
      }
    case 108:
#line 617 "Compiler/bisonin"
      {
	AddCom (11, 2);;
	break;
      }
    case 109:
#line 619 "Compiler/bisonin"
      {
	AddCom (12, 2);;
	break;
      }
    case 110:
#line 621 "Compiler/bisonin"
      {
	AddCom (13, 0);;
	break;
      }
    case 111:
#line 623 "Compiler/bisonin"
      {
	AddCom (17, 1);;
	break;
      }
    case 112:
#line 625 "Compiler/bisonin"
      {
	AddCom (23, 2);;
	break;
      }
    case 113:
#line 627 "Compiler/bisonin"
      {
	AddCom (18, 2);;
	break;
      }
    case 117:
#line 637 "Compiler/bisonin"
      {
	AddComBloc (14, 3, 2);;
	break;
      }
    case 120:
#line 642 "Compiler/bisonin"
      {
	EmpilerBloc ();;
	break;
      }
    case 121:
#line 644 "Compiler/bisonin"
      {
	DepilerBloc (2);;
	break;
      }
    case 122:
#line 645 "Compiler/bisonin"
      {
	DepilerBloc (2);;
	break;
      }
    case 123:
#line 648 "Compiler/bisonin"
      {
	DepilerBloc (1);;
	break;
      }
    case 124:
#line 649 "Compiler/bisonin"
      {
	DepilerBloc (1);;
	break;
      }
    case 125:
#line 653 "Compiler/bisonin"
      {
	AddComBloc (15, 3, 1);;
	break;
      }
    case 126:
#line 657 "Compiler/bisonin"
      {
	AddComBloc (16, 3, 1);;
	break;
      }
    case 127:
#line 662 "Compiler/bisonin"
      {
	AddVar (yyvsp[0].str);;
	break;
      }
    case 128:
#line 664 "Compiler/bisonin"
      {
	AddConstStr (yyvsp[0].str);;
	break;
      }
    case 129:
#line 666 "Compiler/bisonin"
      {
	AddConstStr (yyvsp[0].str);;
	break;
      }
    case 130:
#line 668 "Compiler/bisonin"
      {
	AddConstNum (yyvsp[0].number);;
	break;
      }
    case 131:
#line 670 "Compiler/bisonin"
      {
	AddConstNum (-1);;
	break;
      }
    case 132:
#line 672 "Compiler/bisonin"
      {
	AddConstNum (-2);;
	break;
      }
    case 133:
#line 674 "Compiler/bisonin"
      {
	AddLevelBufArg ();;
	break;
      }
    case 134:
#line 676 "Compiler/bisonin"
      {
	AddFunct (1, 1);;
	break;
      }
    case 135:
#line 677 "Compiler/bisonin"
      {
	AddFunct (2, 1);;
	break;
      }
    case 136:
#line 678 "Compiler/bisonin"
      {
	AddFunct (3, 1);;
	break;
      }
    case 137:
#line 679 "Compiler/bisonin"
      {
	AddFunct (4, 1);;
	break;
      }
    case 138:
#line 680 "Compiler/bisonin"
      {
	AddFunct (5, 1);;
	break;
      }
    case 139:
#line 681 "Compiler/bisonin"
      {
	AddFunct (6, 1);;
	break;
      }
    case 140:
#line 682 "Compiler/bisonin"
      {
	AddFunct (7, 1);;
	break;
      }
    case 141:
#line 683 "Compiler/bisonin"
      {
	AddFunct (8, 1);;
	break;
      }
    case 142:
#line 684 "Compiler/bisonin"
      {
	AddFunct (9, 1);;
	break;
      }
    case 143:
#line 685 "Compiler/bisonin"
      {
	AddFunct (10, 1);;
	break;
      }
    case 144:
#line 686 "Compiler/bisonin"
      {
	AddFunct (11, 1);;
	break;
      }
    case 145:
#line 687 "Compiler/bisonin"
      {
	AddFunct (12, 1);;
	break;
      }
    case 146:
#line 688 "Compiler/bisonin"
      {
	AddFunct (13, 1);;
	break;
      }
    case 147:
#line 689 "Compiler/bisonin"
      {
	AddFunct (14, 1);;
	break;
      }
    case 148:
#line 690 "Compiler/bisonin"
      {
	AddFunct (15, 1);;
	break;
      }
    case 149:
#line 691 "Compiler/bisonin"
      {
	AddFunct (16, 1);;
	break;
      }
    case 150:
#line 692 "Compiler/bisonin"
      {
	AddFunct (17, 1);;
	break;
      }
    case 151:
#line 697 "Compiler/bisonin"
      {;
	break;
      }
    case 178:
#line 743 "Compiler/bisonin"
      {
	l = 1 - 250000;
	AddBufArg (&l, 1);;
	break;
      }
    case 179:
#line 744 "Compiler/bisonin"
      {
	l = 2 - 250000;
	AddBufArg (&l, 1);;
	break;
      }
    case 180:
#line 745 "Compiler/bisonin"
      {
	l = 3 - 250000;
	AddBufArg (&l, 1);;
	break;
      }
    case 181:
#line 746 "Compiler/bisonin"
      {
	l = 4 - 250000;
	AddBufArg (&l, 1);;
	break;
      }
    case 182:
#line 747 "Compiler/bisonin"
      {
	l = 5 - 250000;
	AddBufArg (&l, 1);;
	break;
      }
    case 183:
#line 748 "Compiler/bisonin"
      {
	l = 6 - 250000;
	AddBufArg (&l, 1);;
	break;
      }
    }
  /* the action file gets copied in in place of this dollarsign */
#line 498 "/usr/share/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp - 1)->last_line;
      yylsp->last_column = (yylsp - 1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp + yylen - 1)->last_line;
      yylsp->last_column = (yylsp + yylen - 1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:			/* here on detecting error */

  if (!yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof (yytname) / sizeof (char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen (yytname[x]) + 15, count++;
	  msg = (char *) malloc (size + 15);
	  if (msg != 0)
	    {
	      strcpy (msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof (yytname) / sizeof (char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat (msg, count == 0 ? ", expecting `" : " or `");
			strcat (msg, yytname[x]);
			strcat (msg, "'");
			count++;
		      }
		}
	      yyerror (msg);
	      free (msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("parse error");
    }

  goto yyerrlab1;
yyerrlab1:			/* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf (stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:			/* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];	/* If its default is to accept any token, ok.  Otherwise pop it. */
  if (yyn)
    goto yydefault;
#endif

yyerrpop:			/* pop the current state because it cannot handle the error token */

  if (yyssp == yyss)
    YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf (stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}
#line 751 "Compiler/bisonin"
