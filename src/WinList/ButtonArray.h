/* 
 * Copyright (c) 1998 Rafal Wierzbicki <rafal@mcss.mcmaster.ca>
 * Copyright (c) 1998 Makoto Kato <m_kato@ga2.so-net.ne.jp>
 * Copyright (c) 1998 Rene Fichter <ceezaer@cyberspace.org>
 * Copyright (c) 1997 Guylhem Aznar <guylhem@oeil.qc.ca>
 * Copyright (c) 1994 Mike Finger <mfinger@mermaid.micro.umn.edu>
 *                    or <Mike_Finger@atk.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * $Id: ButtonArray.h,v 1.1 2000/10/20 03:27:57 sashav Exp $
 */

#ifdef I18N
#ifdef __STDC__
#define XTextWidth(x,y,z)	XmbTextEscapement(x ## set,y,z)
#else
#define XTextWidth(x,y,z)	XmbTextEscapement( x/**/set,y,z)
#endif
#define XDrawString(t,u,v,w,x,y,z)	XmbDrawString(t,u,FONTSET,v,w,x,y,z)
#endif

/* Struct definitions */
typedef struct button {
  char *title;
  char *truncate_title; /* valid only if truncatewidth > 0 */
  int up,needsupdate,tw,set,truncatewidth;
	Balloon *balloon;
  struct button *next;
} Button;

typedef struct {
  int count;
  Button *head,*tail;
  int x,y,w,h;
} ButtonArray;

/* Function Prototypes */
void InitArray(ButtonArray *array,int x,int y,int w,int h);
void UpdateArray(MyStyle *Style, ButtonArray *array,int x,int y,int w, int h);
int AddButton(MyStyle *Style, ButtonArray *array, char *title, int up);
int UpdateButton(MyStyle *Style,ButtonArray *array, int butnum, char *title, int up);
void RemoveButton(ButtonArray *array, int butnum);
Button *find_n(ButtonArray *array, int n);
void FreeButton(Button *ptr);
void FreeAllButtons(ButtonArray *array);
void DoButton(MyStyle *Style, Button *ptr, int x, int y, int w, int h);
void DrawButtonArray(MyStyle *Style, ButtonArray *array, int all);
void SwitchButton(MyStyle *Style, ButtonArray *array,int butnum);
int WhichButton(ButtonArray *array,int x, int y);
void PrintButtons(ButtonArray *array);
Button *CurrentButton (ButtonArray *array, int x, int y);

#ifdef I18N
#define DBCS_NOT    0
#define DBCS_LEAD   1
#define DBCS_SECOND 2
int IsDBCSLeadByte(char *s, int i);
#endif
