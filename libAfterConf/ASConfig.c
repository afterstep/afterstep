/*
 * Copyright (c) 2003 Sasha Vasko <sasha at aftercode.net>
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


#define LOCAL_DEBUG

#include "../configure.h"

#include "../libAfterStep/asapp.h"
#include "../libAfterStep/afterstep.h"
#include "../libAfterStep/parser.h"
#include "../libAfterStep/hints.h"

#include "afterconf.h"


/*************************************************************************/
/* Primary syntaxes ( those that are loaded from files )                 */
/* Base : */
extern SyntaxDef     BaseSyntax;

/* Color Scheme : */
extern SyntaxDef     ColorSyntax;

/* Database */
extern SyntaxDef     DatabaseSyntax;

/* Feel : */
extern SyntaxDef 	 FeelSyntax;

/* Autoexec : */
extern SyntaxDef 	 AutoExecSyntax;

/* menu entries and commands : */
extern SyntaxDef 	 FuncSyntax;

/* Theme : */
extern SyntaxDef 	 ThemeSyntax;

/* Look */
extern SyntaxDef     LookSyntax;

/* Pager: */
extern SyntaxDef     PagerSyntax;

/* Wharf: */
extern SyntaxDef     WharfSyntax;

/* WinList: */
extern SyntaxDef     WinListSyntax;

/* WinTab: */
extern SyntaxDef     WinTabsSyntax;
/*************************************************************************/
/* Secondary syntaxes :                                                  */
/* generic */
extern SyntaxDef     GravitySyntax;
extern SyntaxDef     SupportedHintsSyntax;

/* Database */
extern SyntaxDef     StyleSyntax;

/* Feel : */
extern SyntaxDef     PopupFuncSyntax;
extern SyntaxDef     WindowBoxSyntax;

/* Look */
extern SyntaxDef     MyBackgroundSyntax;
extern SyntaxDef     ASetRootSyntax;
extern SyntaxDef     AlignSyntax;
extern SyntaxDef     BevelSyntax;
extern SyntaxDef     MyFrameSyntax;
extern SyntaxDef     MyStyleSyntax;

/* Pager: */
extern SyntaxDef     PagerDecorationSyntax;

/* Wharf: */
extern SyntaxDef     WhevSyntax;

/*************************************************************************/
/*************************************************************************/
typedef struct ASPrimarySyntax
{
	SyntaxDef 	*syntax ; 
	char 		*files[2];


}ASPrimarySyntax;


ASPrimarySyntax AllSyntaxes[] = 
{
	{&BaseSyntax, {"base", NULL}},		
	{NULL, {NULL}}
};	 


TermDef   ASConfigTerms[] = {
    {TF_NO_MYNAME_PREPENDING, "Base", 4, TT_FLAG, ASCONFIG_Base_ID, &BaseSyntax},
	{TF_NO_MYNAME_PREPENDING, "Colorscheme", 11, TT_FLAG, ASCONFIG_Colorscheme_ID, &ColorSyntax},	 
	{TF_NO_MYNAME_PREPENDING, "Database", 8, TT_FLAG, ASCONFIG_Database_ID, &DatabaseSyntax},	 
	{TF_NO_MYNAME_PREPENDING, "AutoExec", 8, TT_FLAG, ASCONFIG_AutoExec_ID, &AutoExecSyntax},	 
	{TF_NO_MYNAME_PREPENDING, "Command", 7, TT_FLAG, ASCONFIG_Command_ID, &FuncSyntax},	 
	{TF_NO_MYNAME_PREPENDING, "Look", 4, TT_FLAG, ASCONFIG_Look_ID, &LookSyntax},	 
	{TF_NO_MYNAME_PREPENDING, "Feel", 4, TT_FLAG, ASCONFIG_Feel_ID, &FeelSyntax},	 
    {TF_NO_MYNAME_PREPENDING, "Theme", 5, TT_FLAG, ASCONFIG_Theme_ID, &ThemeSyntax},	 
	{TF_NO_MYNAME_PREPENDING, "Pager", 5, TT_FLAG, ASCONFIG_Pager_ID, &PagerSyntax},	 
	{TF_NO_MYNAME_PREPENDING, "Wharf", 5, TT_FLAG, ASCONFIG_Wharf_ID, &WharfSyntax},	 
	{TF_NO_MYNAME_PREPENDING, "WinList", 7, TT_FLAG, ASCONFIG_WinList_ID, &WinListSyntax},	 
	{TF_NO_MYNAME_PREPENDING, "WinTabs", 7, TT_FLAG, ASCONFIG_WinTabs_ID, &WinTabsSyntax},	 
	{0, NULL, 0, 0, 0}
};

SyntaxDef ASConfigSyntax =
{
  '\n',
  '\0',
  ASConfigTerms,
  0,				/* use default hash size */
  '\t',
  "",
  "\t",
  "AfterStep Configuration",
  "",
  NULL,
  0
};


