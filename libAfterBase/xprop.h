#ifndef XPROP_H_HEADER_INCLUDED
#define XPROP_H_HEADER_INCLUDED

#include "astypes.h"
#include "output.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************/
/*                Utility data structures :				*/
/************************************************************************/

typedef struct AtomXref
{
  char *name;
  Atom *variable;
  ASFlagType flag;
  Atom atom;
}
AtomXref;

/*************************************************************************/
/*                           Interface                                   */
/*************************************************************************/
/* X Atoms : */
Bool intern_atom_list (AtomXref * list);
void translate_atom_list (ASFlagType *trg, AtomXref * xref, CARD32* list,
                          long nitems);
void print_list_hints( stream_func func, void* stream, ASFlagType flags,
                       AtomXref *xref, const char *prompt );
void encode_atom_list ( AtomXref * xref, CARD32 **list, long *nitems, ASFlagType flags);

Bool read_32bit_proplist (Window w, Atom property, long estimate,
                          CARD32** list, long *nitems);
Bool read_string_property (Window w, Atom property, char **trg);
Bool read_text_property (Window w, Atom property, XTextProperty ** trg);
Bool read_32bit_property (Window w, Atom property, CARD32* trg);

/* This will return contents of X property of XA_INTEGER type
 * in case of success it returns array of unsigned long elements
 * data_size - will hold the size of returned data
 * version   - will hold the version of retrieved data
 */
long *get_as_property ( Window w, Atom property, size_t * data_size, CARD32 *version);
Bool read_as_property ( Window w, Atom property, size_t * data_size, CARD32 *version, CARD32 **trg);

char *text_property2string( XTextProperty *tprop);


void print_text_property( stream_func func, void* stream, XTextProperty *tprop, const char *prompt );
void free_text_property (XTextProperty ** trg);

/* Writing properties : */
void set_32bit_property (Window w, Atom property, Atom type, CARD32 data);
void set_multi32bit_property (Window w, Atom property, Atom type, int items, ...);
void set_32bit_proplist (Window w, Atom property, Atom type, CARD32* list, long nitems);

void set_string_property (Window w, Atom property, char *data);

typedef enum
{
	TPE_String,
	TPE_UTF8
}ASTextEncoding;

void set_text_property (Window w, Atom property, char** data, int items_num, ASTextEncoding encoding);

/* This will set X property of XA_INTEGER type to the array of data of
 * the specifyed size
 */
void set_as_property ( Window w, Atom name, CARD32 *data, size_t data_size, CARD32 version);

#ifdef __cplusplus
}
#endif


#endif /* XPROP_H_HEADER_INCLUDED */
