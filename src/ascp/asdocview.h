#ifndef AS_DOCVIEW_H_HEADER_INCLUDED
#define AS_DOCVIEW_H_HEADER_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif

struct ASDoc;
struct ASView;

typedef struct ASDocType
{
    char *name ;
    
    struct ASDoc* (*create_doc)();
    Bool (*open_file)( struct ASDoc *doc, const char *filename );    
    Bool (*save_file)( struct ASDoc *doc, const char *filename );    
    Bool (*check_sanity)( struct ASDoc *doc );    
    void (*destroy_doc)( struct ASDoc **pdoc );
}ASDocType;

typedef struct ASViewType
{
    char *name ;
    struct ASView* (*create_view)( struct ASDoc *doc);
    Bool           (*open_view)( struct ASView *view);
    void           (*refresh_view)(struct ASView *view);
    Bool           (*close_view)( struct ASView *view);
    void           (*destroy_view)( struct ASView **pview );
    
}ASViewType ;

typedef struct ASDoc
{
    char *name ;

    Bool dirty ;
            
    ASDocType *type ;
    ASVector  *views ;                         /* vector of pointers to ASView s */
    ASMagic *data ;
}ASDoc;

typedef struct ASView
{
    ASViewType *type ;
    
    ASDoc *doc ;
    ASMagic *data ;
}ASView ;    

typedef struct ASDocManager
{
    ASVector *docs ;                      /* array of pointers to open documents */
    
    
}ASDocManager;

typedef struct ASViewFrame
{
    ASVector *views ;                      /* array of pointers to open views */
    
    
}ASViewFrame;

#ifdef __cplusplus
}
#endif


#endif

