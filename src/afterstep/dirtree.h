#ifndef DIRTREE_H
#define DIRTREE_H

#include "../../libAfterStep/functions.h"

typedef struct dirtree_t
{
  struct dirtree_t* next;
  struct dirtree_t* parent;
  struct dirtree_t* child;

  char* name;
  char* stripped_name;
  char* path;
  int flags;
  time_t mtime;

  FunctionData command;
  char* icon;
  char* extension;
  char* minipixmap_extension;
  int order;
  int recent_items;
} dirtree_t;

/* flags for dirtree_t.flags */
enum
{
  DIRTREE_ID		= (1 << 16) - 1, /* ID assigned to this menu */
  DIRTREE_DIR		= (1 << 16),	 /* if this item a submenu */
  DIRTREE_KEEPNAME	= (1 << 17),	 /* if we should keep the menu name for the PopUp command */
  DIRTREE_MINIPIXMAP = (1 << 18),
  DIRTREE_RECENT_ITEMS_SET=(1 << 19), 
  DIRTREE_LAST
};

dirtree_t* dirtree_new(void);
dirtree_t* dirtree_new_from_dir(const char* dir);
void dirtree_remove(dirtree_t* tree);
void dirtree_delete(dirtree_t* tree);
void dirtree_fill_from_dir(dirtree_t* tree);
void dirtree_move_children(dirtree_t* tree1, dirtree_t* tree2);
void dirtree_set_command(dirtree_t* tree, struct FunctionData* command, int recurse);
void dirtree_parse_include(dirtree_t* tree);
void dirtree_remove_order(dirtree_t* tree);
void dirtree_merge(dirtree_t* tree);
void dirtree_sort(dirtree_t* tree);
void dirtree_output_tree(FILE* fp, dirtree_t* tree, int recurse);
int dirtree_parse(dirtree_t* tree, const char* file);
int dirtree_set_id(dirtree_t* tree, int id);

int dirtree_compar(const dirtree_t** d1, const dirtree_t** d2);
int dirtree_compar_order(const dirtree_t** d1, const dirtree_t** d2);
int dirtree_compar_type(const dirtree_t** d1, const dirtree_t** d2);
int dirtree_compar_alpha(const dirtree_t** d1, const dirtree_t** d2);
int dirtree_compar_mtime(const dirtree_t** d1, const dirtree_t** d2);

typedef int (*dirtree_compar_f) (const dirtree_t** d1, const dirtree_t** d2);

extern dirtree_compar_f dirtree_compar_list[5];

#endif /* DIRTREE_H */
