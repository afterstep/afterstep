#ifndef OS_H_HEADER_INCLUDED
#define OS_H_HEADER_INCLUDED

#define MAXHOSTNAME 255

Bool  mygethostname (char *client, size_t length);
char* mygetostype (void);
int   GetFdWidth (void);


#endif /* OS_H_HEADER_INCLUDED */
