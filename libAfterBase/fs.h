#ifndef FS_H_HEADER_INCLUDED
#define FS_H_HEADER_INCLUDED

time_t  FileModifiedTime (const char *filename);
int		CheckMode (const char *file, int mode);

#if !defined(S_IFREG) || !defined(S_IFDIR)
#include <sys/stat.h>
#endif

#define CheckFile(f) 	CheckMode(f,S_IFREG)
#define CheckDir(d) 	CheckMode(d,S_IFDIR)

int     CopyFile (const char *realfilename1, const char *realfilename2);

char   *find_envvar (char *var_start, int *end_pos);
void	replaceEnvVar (char **path);

char   *make_file_name (const char *path, const char *file);
char   *PutHome (const char *path_with_home);
char   *find_file (const char *file, const char *pathlist, int type);
int 	is_executable_in_path (const char *name);

#endif
