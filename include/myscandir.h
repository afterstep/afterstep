#ifndef MYSCANDIR_H
#define MYSCANDIR_H

#include <sys/types.h>
#include <dirent.h>

struct direntry
  {
    mode_t d_mode;		/* S_IFDIR if a directory */
    time_t d_mtime;
    char d_name[1];
  };

typedef int (*my_sort_f) (struct direntry ** d1, struct direntry ** d2);

int my_scandir (char *, struct direntry *(*[]), int (*select) (struct dirent *),
		my_sort_f dcomp);
int ignore_dots (struct dirent *e);

#endif /* MYSCANDIR_H */
