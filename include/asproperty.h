#ifndef AS_PROPERTY_H
#define AS_PROPERTY_H

/* This will set X property of XA_INTEGER type to the array of data of 
 * the specifyed size
 */
void set_as_property (Display * dpy, Window w, Atom name, unsigned long *data, size_t data_size, unsigned long version);

/* This will return contents of X property of XA_INTEGER type 
 * in case of success it returns array of unsigned long elements
 * data_size - will hold the size of returned data
 * version   - will hold the version of retrieved data 
 *
 * NOTE: returned data must be freed using XFree() !!!!
 *
 */
unsigned long *get_as_property (Display * dpy, Window w, Atom name, size_t * data_size, unsigned long *version);

#endif /* AS_PROPERTY_H */
