#ifndef FS_H_HEADER_INCLUDED
#define FS_H_HEADER_INCLUDED

#include <time.h>

#include <sys/types.h>

#if !defined(S_IFREG) || !defined(S_IFDIR)
#include <sys/stat.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct dirent;
struct stat;
struct direntry
  {
    mode_t d_mode;		/* S_IFDIR if a directory */
    time_t d_mtime;
	off_t  d_size;		/* total size, in bytes */
    char d_name[1];
  };

typedef int (*my_sort_f) (struct direntry ** d1, struct direntry ** d2);

int
my_scandir_ext ( const char *dirname, int (*filter_func) (const char *),
				 Bool (*handle_direntry_func)( const char *fname, const char *fullname, struct stat *stat_info, void *aux_data), 
				 void *aux_data);
int my_scandir (char *, struct direntry *(*[]), int (*select) (const char *),
		my_sort_f dcomp);

int ignore_dots (const char *dname);
int no_dots_except_include (const char *d_name);
int no_dots_except_directory (const char *d_name);


time_t  get_file_modified_time (const char *filename);
int		check_file_mode (const char *file, int mode);

#define CheckFile(f) 	check_file_mode(f,S_IFREG)
#define CheckDir(d) 	check_file_mode(d,S_IFDIR)

int     copy_file (const char *realfilename1, const char *realfilename2);
char*	load_binary_file(const char* realfilename, long *file_size_return);
char*   load_file (const char *realfilename);

/* char   *find_envvar (char *var_start, int *end_pos); */
void	replace_envvar (char **path);
char   *copy_replace_envvar (const char *path);

void	parse_file_name(const char *filename, char **path, char **file);
char   *make_file_name (const char *path, const char *file);
char   *put_file_home (const char *path_with_home);
char   *find_file (const char *file, const char *pathlist, int type);
int 	is_executable_in_path (const char *name);
int		my_scandir (char *dirname, struct direntry *(*namelist[]),
					int (*select) (const char *),
					int (*dcomp) (struct direntry **, struct direntry **));

#ifdef __cplusplus
}
#endif

#endif
