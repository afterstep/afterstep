/*
 * Copyright (C) 1999 Ethan Fischer <allanon@crystaltokyo.com>
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
 *
 */

#include "../configure.h"

#ifdef DEBUG_ALLOCS

#include "asapp.h"

#undef malloc
#undef safemalloc
#undef calloc
#undef realloc
#undef free

#undef XCreatePixmap
#undef XCreateBitmapFromData
#undef XCreatePixmapFromBitmapData
#undef XFreePixmap

#undef XCreateGC
#undef XFreeGC

#undef XCreateImage
#undef XGetImage
#undef XDestroyImage

#undef XpmReadFileToPixmap
#undef XpmCreateImageFromXpmImage

#undef XGetWindowProperty
#undef XQueryTree
#undef XGetWMHints
#undef XGetWMProtocols
#undef XGetWMName
#undef XGetClassHint
#undef XGetAtomName
#undef XStringListToTextProperty
#undef XFree

void
log_call (const char *fname, int line, const char *call_name, const char *sparam)
{
  fprintf (stderr, "%s(%s) called from %s:%d\n", call_name, sparam, fname, line);
}


#define MAX_AUDIT_ALLOCS 30000

typedef struct mem
{
  struct mem *next;
  const char *fname;
  int line;
  size_t length;
  int type;
  void *ptr;
  char freed;
}
mem;

enum
  {
    C_MEM = 0,
    C_MALLOC = 0x100,
    C_CALLOC = 0x200,
    C_REALLOC = 0x300,

    C_PIXMAP = 1,
    C_CREATEPIXMAP = 0x100,
    C_XPMFILE = 0x200,
    C_BITMAPFROMDATA = 0x300,
    C_FROMBITMAP = 0x400,

    C_GC = 2,

    C_IMAGE = 3,
    C_GETIMAGE = 0x100,
    /*    C_XPMFILE = 0x200, *//* must be same as pixmap version above */

    C_XMEM = 4,
    C_XGETWINDOWPROPERTY = 0x100,
    C_XQUERYTREE = 0x200,
    C_XGETWMHINTS = 0x300,
    C_XGETWMPROTOCOLS = 0x400,
    C_XGETWMNAME = 0x500,
    C_XGETCLASSHINT = 0x600,
    C_XGETATOMNAME = 0x700,
    C_XSTRINGLISTTOTEXTPROPERTY = 0x800,

    C_LAST_TYPE
  };

static mem *first_mem = NULL;
static int allocations = 0;
static int reallocations = 0;
static int deallocations = 0;
static int max_allocations = 0;
static int max_alloc = 0;
static int max_x_alloc = 0;
static int total_alloc = 0;
static int total_x_alloc = 0;

void count_alloc (const char *fname, int line, void *ptr, size_t length, int type);
mem *count_find (const char *fname, int line, void *ptr, int type, mem ** pm2);
mem *count_find_and_extract (const char *fname, int line, void *ptr, int type);

void
count_alloc (const char *fname, int line, void *ptr, size_t length, int type)
{
  mem *m;
  m = (mem *) malloc (sizeof (mem));
  memset (m, 0, sizeof (mem));
  m->next = first_mem;
  first_mem = m;
  m->fname = fname;
  m->line = line;
  m->length = length;
  m->type = type;
  m->ptr = ptr;
  m->freed = 0;
  allocations++;
  if ((type & 0xff) == C_MEM)
    {
      total_alloc += length;
      if (total_alloc > max_alloc)
	max_alloc = total_alloc;
    }
  else
    {
      total_x_alloc += length;
      if (total_x_alloc > max_x_alloc)
	max_x_alloc = total_x_alloc;
    }
  if (allocations - deallocations > max_allocations)
    max_allocations = allocations - deallocations;
}

mem *
count_find (const char *fname, int line, void *ptr, int type, mem ** pm2)
{
  mem *m1;
  mem *m2;
  /* scan for an invalid allocation list */
  {
    int q;
    mem *mmm;
    for (q = 0, mmm = first_mem; mmm != NULL && q < MAX_AUDIT_ALLOCS; mmm = mmm->next, q++);
    if (q == MAX_AUDIT_ALLOCS)
      {
	fprintf (stderr, "%s:maximum number of memory allocation has reached in (%s:%d)\n", __FUNCTION__, fname, line);
	exit (1);
      }
  }

  for (m1 = m2 = first_mem; m1 != NULL; m2 = m1, m1 = m1->next)
    if (m1->ptr == ptr && (m1->type & 0xff) == (type & 0xff))
      break;
  if (pm2)
    *pm2 = m2;
  return m1;
}

mem *
count_find_and_extract (const char *fname, int line, void *ptr, int type)
{
  mem *m1;
  mem *m2;

  if ((m1 = count_find (fname, line, ptr, type, &m2)) == NULL)
    return NULL;

  if (m1 == first_mem)
    first_mem = m1->next;
  else
    m2->next = m1->next;

  /* scan for an invalid allocation list */
  {
    int q;
    mem *mmm;
    for (q = 0, mmm = first_mem; mmm != NULL && q < MAX_AUDIT_ALLOCS; mmm = mmm->next, q++);
    if (q == MAX_AUDIT_ALLOCS)
      {
	fprintf (stderr, "%s:maximum number of memory allocation has reached in (%s:%d)\n", __FUNCTION__, fname, line);
	exit (1);
      }
  }

  if ((m1->type & 0xff) == C_MEM)
    {
      total_alloc -= m1->length;
    }
  else
    {
      total_x_alloc -= m1->length;
    }

  deallocations++;

  return m1;
}

void *
countmalloc (const char *fname, int line, size_t length)
{
  void *ptr = safemalloc (length);
  count_alloc (fname, line, ptr, length, C_MEM | C_MALLOC);
  return ptr;
}

void *
countcalloc (const char *fname, int line, size_t nrecords, size_t length)
{
  void *ptr = calloc (nrecords, length);
  count_alloc (fname, line, ptr, nrecords * length, C_MEM | C_CALLOC);
  return ptr;
}

void *
countrealloc (const char *fname, int line, void *ptr, size_t length)
{
  if (ptr != NULL && length == 0)
    countfree (fname, line, ptr);
  if (length == 0)
    return NULL;
  if (ptr != NULL)
    {
      mem *m;
      for (m = first_mem; m != NULL; m = m->next)
	if (m->ptr == ptr && (m->type & 0xff) == C_MEM)
	  break;
      if (m == NULL)
	{
	  fprintf (stderr, "%s:mem to realloc not in list!\n", __FUNCTION__);
	  fprintf (stderr, "%s:called from %s:%d\n", __FUNCTION__, fname, line);
	  return NULL;
	  /* exit (1); */
	}
      if ((m->type & 0xff) == C_MEM)
	{
	  total_alloc -= m->length;
	  total_alloc += length;
	  if (total_alloc > max_alloc)
	    max_alloc = total_alloc;
	}
      else
	{
	  total_x_alloc -= m->length;
	  total_x_alloc += length;
	  if (total_x_alloc > max_x_alloc)
	    max_x_alloc = total_x_alloc;
	}
      m->fname = fname;
      m->line = line;
      m->length = length;
      m->type = C_MEM | C_REALLOC;
      m->ptr = realloc (ptr, length);
      m->freed = 0;
      ptr = m->ptr;

      reallocations++;
    }
  else
    ptr = countmalloc (fname, line, length);

  return ptr;
}

void
countfree (const char *fname, int line, void *ptr)
{
  mem *m = count_find_and_extract (fname, line, ptr, C_MEM);

  if (m == NULL)
    {
      fprintf (stderr, "%s:mem to free not in list!\n", __FUNCTION__);
      fprintf (stderr, "%s:called from %s:%d\n", __FUNCTION__, fname, line);
      return;
    }

#if 0
// this is invalid code!!
  if (m1->freed > 0)
    {
      fprintf (stderr, "%s:mem already freed %d time(s)!\n", __FUNCTION__, m1->freed);
      fprintf (stderr, "%s:freed from %s:%d\n", __FUNCTION__, (*m1).fname, (*m1).line);
      fprintf (stderr, "%s:called from %s:%d\n", __FUNCTION__, fname, line);
      /* exit (1); */
    }
  else
    safefree (m1->ptr);
  m1->freed++;
  m1->fname = fname;
  m1->line = line;
#else
  safefree (m->ptr);
  safefree (m);
#endif
}

void
print_unfreed_mem (void)
{
  mem *m;
  int q;
  fprintf (stderr, "===============================================================================\n");
  fprintf (stderr, "Memory audit: %s\n", MyName);
  fprintf (stderr, "\n");
  fprintf (stderr, "   Total   allocs: %d\n", allocations);
  fprintf (stderr, "   Total reallocs: %d\n", reallocations);
  fprintf (stderr, "   Total deallocs: %d\n", deallocations);
  fprintf (stderr, "Max allocs at any one time: %d\n", max_allocations);
  fprintf (stderr, "      Lost memory: %d\n", total_alloc);
  fprintf (stderr, "    Lost X memory: %d\n", total_x_alloc);
  fprintf (stderr, "  Max memory used: %d\n", max_alloc);
  fprintf (stderr, "Max X memory used: %d\n", max_x_alloc);
  fprintf (stderr, "\n");
  fprintf (stderr, "List of unfreed memory\n");
  fprintf (stderr, "----------------------\n");
  fprintf (stderr, "allocating function    |line |length |pointer    |type (subtype)\n");
  fprintf (stderr, "-----------------------+-----+-------+-----------+--------------\n");
  for (q = 0, m = first_mem; m != NULL && q < MAX_AUDIT_ALLOCS; m = m->next, q++)
    if (m->freed == 0)
      {
	fprintf (stderr, "%23s|%-5d|%-7d|0x%08x ", m->fname, m->line, m->length, (unsigned int) m->ptr);
	switch (m->type & 0xff)
	  {
	  case C_MEM:
	    fprintf (stderr, "| malloc");
	    switch (m->type & ~0xff)
	      {
	      case C_MALLOC:
		fprintf (stderr, " (malloc)");
		break;
	      case C_CALLOC:
		fprintf (stderr, " (calloc)");
		break;
	      case C_REALLOC:
		fprintf (stderr, " (realloc)");
		break;
	      }
	    /* if it seems to be a string, print it */
	    {
	      int i;
	      const unsigned char *ptr = m->ptr;
	      for (i = 0; i < m->length; i++)
		{
		  if (ptr[i] == '\0')
		    break;
		  /* don't print strings containing non-space control characters or high ASCII */
		  if ((ptr[i] <= 0x20 && !isspace (ptr[i])) || ptr[i] >= 0x80)
		    i = m->length;
		}
	      if (i < m->length)
		fprintf (stderr, " '%s'", ptr);
	    }
	    break;
	  case C_PIXMAP:
	    fprintf (stderr, "| pixmap");
	    switch (m->type & ~0xff)
	      {
	      case C_CREATEPIXMAP:
		fprintf (stderr, " (XCreatePixmap)");
		break;
	      case C_XPMFILE:
		fprintf (stderr, " (XpmReadFileToPixmap)");
		break;
	      case C_BITMAPFROMDATA:
		fprintf (stderr, " (XCreateBitmapFromData)");
		break;
	      case C_FROMBITMAP:
		fprintf (stderr, " (XCreatePixmapFromBitmapData)");
		break;
	      }
	    break;
	  case C_GC:
	    fprintf (stderr, "| gc (XCreateGC)");
	    break;
	  case C_IMAGE:
	    fprintf (stderr, "| image");
	    switch (m->type & ~0xff)
	      {
	      case 0:
		fprintf (stderr, " (XCreateImage)");
		break;
	      case C_GETIMAGE:
		fprintf (stderr, " (XGetImage)");
		break;
	      case C_XPMFILE:
		fprintf (stderr, " (XpmCreateImageFromXpmImage)");
		break;
	      }
	    break;
	  case C_XMEM:
	    fprintf (stderr, "| X mem");
	    switch (m->type & ~0xff)
	      {
	      case C_XGETWINDOWPROPERTY:
		fprintf (stderr, " (XGetWindowProperty)");
		break;
	      case C_XQUERYTREE:
		fprintf (stderr, " (XQueryTree)");
		break;
	      case C_XGETWMHINTS:
		fprintf (stderr, " (XGetWMHints)");
		break;
	      case C_XGETWMPROTOCOLS:
		fprintf (stderr, " (XGetWMProtocols)");
		break;
	      case C_XGETWMNAME:
		fprintf (stderr, " (XGetWMName)");
		break;
	      case C_XGETCLASSHINT:
		fprintf (stderr, " (XGetClassHint)");
		break;
	      case C_XGETATOMNAME:
		fprintf (stderr, " (XGetAtomName)");
		break;
	      case C_XSTRINGLISTTOTEXTPROPERTY:
		fprintf (stderr, " (XStringListToTextProperty)");
		break;
	      }
	    break;
	  }
	fprintf (stderr, "\n");
      }
  fprintf (stderr, "===============================================================================\n");
}

Pixmap
count_xcreatepixmap (const char *fname, int line, Display * display, Drawable drawable, unsigned int width, unsigned int height, unsigned int depth)
{
  Pixmap pmap = XCreatePixmap (display, drawable, width, height, depth);
  if (pmap == None)
    return None;
  count_alloc (fname, line, (void *) pmap, width * height * depth / 8, C_PIXMAP | C_CREATEPIXMAP);
  return pmap;
}

Pixmap
count_xcreatebitmapfromdata (const char *fname, int line, Display * display, Drawable drawable, char *data, unsigned int width, unsigned int height)
{
  Pixmap pmap = XCreateBitmapFromData (display, drawable, data, width, height);
  if (pmap == None)
    return None;
  count_alloc (fname, line, (void *) pmap, width * height / 8, C_PIXMAP | C_BITMAPFROMDATA);
  return pmap;
}

Pixmap
count_xcreatepixmapfrombitmapdata (const char *fname, int line, Display * display, Drawable drawable, char *data, unsigned int width, unsigned int height, unsigned long fg, unsigned long bg, unsigned int depth)
{
  Pixmap pmap = XCreatePixmapFromBitmapData (display, drawable, data, width, height, fg, bg, depth);
  if (pmap == None)
    return None;
  count_alloc (fname, line, (void *) pmap, width * height * depth / 8, C_PIXMAP | C_FROMBITMAP);
  return pmap;
}

int
count_xfreepixmap (const char *fname, int line, Display * display, Pixmap pmap)
{
  mem *m = count_find_and_extract (fname, line, (void *) pmap, C_PIXMAP);

  if (m == NULL)
    {
      fprintf (stderr, "%s:mem to free not in list!\n", __FUNCTION__);
      fprintf (stderr, "%s:called from %s:%d\n", __FUNCTION__, fname, line);
      return !Success;
    }

  XFreePixmap (display, pmap);
  safefree (m);
  return Success;
}

GC
count_xcreategc (const char *fname, int line, Display * display, Drawable drawable, unsigned int mask, XGCValues * values)
{
  GC gc = XCreateGC (display, drawable, mask, values);
  if (gc == None)
    return None;
  count_alloc (fname, line, (void *) gc, sizeof (XGCValues), C_GC);
  return gc;
}

int
count_xfreegc (const char *fname, int line, Display * display, GC gc)
{
  mem *m = count_find_and_extract (fname, line, (void *) gc, C_GC);

  if (m == NULL)
    {
      fprintf (stderr, "%s:mem to free not in list!\n", __FUNCTION__);
      fprintf (stderr, "%s:called from %s:%d\n", __FUNCTION__, fname, line);
      return !Success;
    }

  XFreeGC (display, gc);
  safefree (m);
  return Success;
}

XImage *
count_xcreateimage (const char *fname, int line, Display * display, Visual * visual, unsigned int depth, int format, int offset, char *data, unsigned int width, unsigned int height, int bitmap_pad, int byte_per_line)
{
  XImage *image = XCreateImage (display, visual, depth, format, offset, data, width, height, bitmap_pad, byte_per_line);
  if (image == NULL)
    return NULL;
  count_alloc (fname, line, (void *) image, sizeof (*image), C_IMAGE);
  return image;
}

XImage *
count_xgetimage (const char *fname, int line, Display * display, Drawable drawable, int x, int y, unsigned int width, unsigned int height, unsigned long plane_mask, int format)
{
  XImage *image = XGetImage (display, drawable, x, y, width, height, plane_mask, format);
  if (image == NULL)
    return NULL;
  count_alloc (fname, line, (void *) image, sizeof (*image) + image->height * image->bytes_per_line, C_IMAGE | C_GETIMAGE);
  return image;
}

#ifdef XPM
int
count_xpmreadfiletopixmap (const char *fname, int line, Display * display, Drawable drawable, char *filename, Pixmap * pmap, Pixmap * mask, void *attributes)
{
  XpmAttributes *xpm_attributes = attributes;
  int val;
  val = XpmReadFileToPixmap (display, drawable, filename, pmap, mask, xpm_attributes);
  if (pmap != NULL && *pmap != None)
    {
      size_t size = 0;
      if (xpm_attributes->valuemask & XpmSize)
	size = xpm_attributes->width * xpm_attributes->height * DefaultDepth (display, DefaultScreen (display)) / 8;
      count_alloc (fname, line, (void *) *pmap, size, C_PIXMAP | C_XPMFILE);
    }
  if (mask != NULL && *mask != None)
    {
      size_t size = 0;
      if (xpm_attributes->valuemask & XpmSize)
	size = xpm_attributes->width * xpm_attributes->height * 1 / 8;
      count_alloc (fname, line, (void *) *mask, size, C_PIXMAP | C_XPMFILE);
    }
  return val;
}

int
count_xpmcreateimagefromxpmimage (const char *fname, int line, Display * display, void *xpm_image_v, XImage ** image, XImage ** mask, void *attributes)
{
  XpmAttributes *xpm_attributes = attributes;
  XpmImage *xpm_image = xpm_image_v;
  int val;
  val = XpmCreateImageFromXpmImage (display, xpm_image, image, mask, xpm_attributes);
  if (image != NULL && *image != NULL)
    {
      size_t size = 0;
      if (xpm_attributes->valuemask & XpmSize)
	size = xpm_attributes->width * xpm_attributes->height * DefaultDepth (display, DefaultScreen (display)) / 8;
      count_alloc (fname, line, (void *) *image, size, C_IMAGE | C_XPMFILE);
    }
  if (mask != NULL && *mask != NULL)
    {
      size_t size = 0;
      if (xpm_attributes->valuemask & XpmSize)
	size = xpm_attributes->width * xpm_attributes->height * 1 / 8;
      count_alloc (fname, line, (void *) *mask, size, C_IMAGE | C_XPMFILE);
    }
  return val;
}
#endif /* XPM */

int
count_xdestroyimage (const char *fname, int line, XImage * image)
{
  mem *m;
  void *image_data = (void *) (image->data), *image_obdata = (void *) (image->obdata);

  if ((m = count_find (fname, line, (void *) image, C_IMAGE, NULL)) == NULL)
    /* can also be of C_MEM type if we allocated it ourselvs */
    if ((m = count_find (fname, line, (void *) image, C_MEM, NULL)) == NULL)
      {
	fprintf (stderr, "%s:mem to free not in list!\n", __FUNCTION__);
	fprintf (stderr, "%s:called from %s:%d\n", __FUNCTION__, fname, line);
	return !Success;
      }

  (*image->f.destroy_image) (image);

  if ((m = count_find_and_extract (fname, line, (void *) image, C_IMAGE)) == NULL)
    /* can also be of C_MEM type if we allocated it ourselvs */
    m = count_find_and_extract (fname, line, (void *) image, C_MEM);
  if (m)
    safefree (m);

  /* find and free the image->data pointer if it is in our list */
  if ((m = count_find_and_extract (fname, line, image_data, C_MEM)) != NULL)
    safefree (m);

  /* find and free the image->obdata pointer if it is in our list */
  if ((m = count_find_and_extract (fname, line, image_obdata, C_MEM)) != NULL)
    safefree (m);

  return Success;
}

int
count_xgetwindowproperty (const char *fname, int line, Display * display, Window w, Atom property, long long_offset, long long_length, Bool delete, Atom req_type, Atom * actual_type_return, int *actual_format_return, unsigned long *nitems_return, unsigned long *bytes_after_return, unsigned char **prop_return)
{
  int val;
  unsigned long my_nitems_return;
  val = XGetWindowProperty (display, w, property, long_offset, long_length, delete, req_type, actual_type_return,
  actual_format_return, &my_nitems_return, bytes_after_return, prop_return);
  if (val == Success && my_nitems_return)
    count_alloc (fname, line, (void *) *prop_return, my_nitems_return * *actual_format_return / 8, C_XMEM | C_XGETWINDOWPROPERTY);
  *nitems_return = my_nitems_return;	/* need to do this in case bytes_after_return and nitems_return point to the same var */
  return val;
}

Status
count_xquerytree (const char *fname, int line, Display * display, Window w, Window * root_return, Window * parent_return, Window ** children_return, unsigned int *nchildren_return)
{
  Status val;
  val = XQueryTree (display, w, root_return, parent_return, children_return, nchildren_return);
  if (val && *nchildren_return)
    count_alloc (fname, line, (void *) *children_return, *nchildren_return * sizeof (Window), C_XMEM | C_XQUERYTREE);
  return val;
}

/* really returns XWMHints*, but to avoid requiring extra includes, we'll return void* */
void *
count_xgetwmhints (const char *fname, int line, Display * display, Window w)
{
  XWMHints *val;
  val = XGetWMHints (display, w);
  if (val != NULL)
    count_alloc (fname, line, (void *) val, sizeof (XWMHints), C_XMEM | C_XGETWMHINTS);
  return (void *) val;
}

/* protocols_return is really Atom**, but to avoid extra includes, we'll use void* */
Status
count_xgetwmprotocols (const char *fname, int line, Display * display, Window w, void *protocols_return, int *count_return)
{
  Status val;
  val = XGetWMProtocols (display, w, (Atom **) protocols_return, count_return);
  if (val && *count_return)
    count_alloc (fname, line, *(void **) protocols_return, *count_return * sizeof (Atom), C_XMEM | C_XGETWMPROTOCOLS);
  return val;
}

/* text_prop_return is really XTextProperty*, but to avoid extra includes, we'll use void* */
Status
count_xgetwmname (const char *fname, int line, Display * display, Window w, void *text_prop_return)
{
  Status val;
  XTextProperty *prop = text_prop_return;
  val = XGetWMName (display, w, prop);
  if (val && prop->nitems)
    count_alloc (fname, line, (void *) prop->value, prop->nitems * prop->format / 8, C_XMEM | C_XGETWMNAME);
  return val;
}

/* class_hints_return is really XTextProperty*, but to avoid extra includes, we'll use void* */
Status
count_xgetclasshint (const char *fname, int line, Display * display, Window w, void *class_hint_return)
{
  Status val;
  XClassHint *prop = class_hint_return;
  val = XGetClassHint (display, w, prop);
  if (val && prop->res_name)
    count_alloc (fname, line, (void *) prop->res_name, strlen (prop->res_name), C_XMEM | C_XGETCLASSHINT);
  if (val && prop->res_class)
    count_alloc (fname, line, (void *) prop->res_class, strlen (prop->res_class), C_XMEM | C_XGETCLASSHINT);
  return val;
}

char *
count_xgetatomname (const char *fname, int line, Display * display, Atom atom)
{
  char *val = XGetAtomName (display, atom);
  if (val != NULL)
    count_alloc (fname, line, (void *) val, strlen (val), C_XMEM | C_XGETATOMNAME);
  return val;
}

/* text_prop_return is really XTextProperty*, but to avoid extra includes, we'll use void* */
Status
count_xstringlisttotextproperty (const char *fname, int line, char **list, int count, void *text_prop_return)
{
  Status val;
  XTextProperty *prop = text_prop_return;
  val = XStringListToTextProperty (list, count, prop);
  if (val && prop->nitems)
    count_alloc (fname, line, (void *) prop->value, prop->nitems * prop->format / 8, C_XMEM | C_XSTRINGLISTTOTEXTPROPERTY);
  return val;
}

int
count_xfree (const char *fname, int line, void *data)
{
  mem *m = count_find_and_extract (fname, line, (void *) data, C_XMEM);

  if (m == NULL)
    {
      fprintf (stderr, "%s:mem to free not in list! (%p)\n", __FUNCTION__, data);
      fprintf (stderr, "%s:called from %s:%d\n", __FUNCTION__, fname, line);
      return !Success;
    }

  XFree (data);
  safefree (m);
  return Success;
}

#endif /* DEBUG_ALLOCS */
