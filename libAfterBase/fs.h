#ifndef FS_H_HEADER_INCLUDED
#define FS_H_HEADER_INCLUDED

#include <time.h>
time_t  get_file_modified_time (const char *filename);
int		check_file_mode (const char *file, int mode);

#if !defined(S_IFREG) || !defined(S_IFDIR)
#include <sys/stat.h>
#endif

#define CheckFile(f) 	check_file_mode(f,S_IFREG)
#define CheckDir(d) 	check_file_mode(d,S_IFDIR)

int     copy_file (const char *realfilename1, const char *realfilename2);

char   *find_envvar (char *var_start, int *end_pos);
void	replace_envvar (char **path);
char   *copy_replace_envvar (char *path);

char   *make_file_name (const char *path, const char *file);
char   *put_file_home (const char *path_with_home);
char   *find_file (const char *file, const char *pathlist, int type);
int 	is_executable_in_path (const char *name);

#endif
