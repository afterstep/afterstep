#ifndef OS_H_HEADER_INCLUDED
#define OS_H_HEADER_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define MAXHOSTNAME 255

Bool  mygethostname (char *client, size_t length);
char* mygetostype (void);
int   get_fd_width (void);

#ifdef __cplusplus
}
#endif


#endif /* OS_H_HEADER_INCLUDED */
