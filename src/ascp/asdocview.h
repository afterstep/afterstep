#ifndef AS_DOCVIEW_H_HEADER_INCLUDED
#define AS_DOCVIEW_H_HEADER_INCLUDED

#include <vector>

using namespace std;

class ASDoc;
class ASDocManager;
class ASView;
class ASViewManager;
struct ASEvent ;
struct ASHashTable ;

class ASDocType
{  /* used in object factory to constract subtypes of ASDoc 
    * objects inheriting it should only redefine CreateDoc() method */
        char *m_Name ;
        Atom  m_Magic ;
    public : 
        ASDocType( const char *name );
        virtual ~ASDocType(); 

        Atom GetMagic() { return m_Magic; };

        virtual ASDoc*  CreateDoc(const char *name) = 0;
        friend class ASDocManager ; 
};

class ASDoc
{
        char      *m_Name ;

    protected :

        char *m_Filename ;
        Bool  m_Dirty ;
            
        vector<ASView*> m_Views ;                         /* vector of pointers to ASView s */
    public :

        ASDoc( const char *name ); 
        virtual ~ASDoc();
            
        virtual Bool    OpenFile( const char *filename ) = 0;    
        virtual Bool    SaveFile( const char *filename = NULL ) = 0;    
        virtual Bool    CheckSanity() = 0;    

        void AttachView( ASView *view );
        void DetachView( ASView *view );
        void RefreshAllViews();
};

class ASViewType
{  /* used in object factory to constract subtypes of ASView 
    * objects inheriting it should only redefine CreateView() method */
        char *m_Name ;
        Atom  m_Magic ;

    public :
        
        ASViewType( const char *name );
        virtual ~ASViewType();
        
        Atom GetMagic() { return m_Magic; };

        virtual ASView* CreateView( ASDoc *doc, ASView *parent ) = 0;

        friend class ASViewManager ;
};

class ASView
{

        Window m_Window ; 
  
        /* this are here for reference and should never be destroyed from ASView class */
        ASDoc   *m_Doc ;
        ASView  *m_Parent ;
        
        Bool  m_Open ;
    public : 
        
        ASView( ASDoc *doc, ASView *parent ); 
        virtual ~ASView();
        
        virtual Bool    Open() = 0;
        virtual void    Refresh() = 0;
        virtual void    Close() { m_Open = False; };
        virtual Bool    HandleXEvent(struct ASEvent *event) = 0;


        Window WindowXID(){ return m_Window; };
};    

class ASDocViewTemplate
{
        char    *m_DocTypeName ;
        char    *m_ViewTypeName ; 
    public : 
        ASDocViewTemplate( const char *doc_type, const char *view_type );
        ~ASDocViewTemplate();

        const char *GetDocTypeName() { return m_DocTypeName; };
        const char *GetViewTypeName() { return m_ViewTypeName; };
};
    

/* document manager is actually a data for ASDoc, that gets attached to 
 * Main Frame which is in turn inherited from ASView
 */

class ASDocManager : public ASDoc
{
        /* this is static data that defines applicatrion at compile time and should not be deallocated */
        struct ASHashTable *m_DocTypes ;              /* hashed by name */
 
        /* this structures are allocated in run time and as such should be deallocated on exit */
        vector<ASDoc*>  m_Docs ;                      /* array of pointers to open documents */
    public :
        ASDocManager();
        ~ASDocManager();       

        /* ASDoc virtual methods : */
        Bool    OpenFile( const char *filename ) { return False; };    
        Bool    SaveFile( const char *filename = NULL ){ return False; };    
        Bool    CheckSanity(){ return True; };
        
        /* Doc manager specific stuff : */
        Bool    RegisterDocType( ASDocType *type );     
        ASDocType*  LookupDocType( const char *doc_type_name );

        ASDoc*  CreateNewDoc(  const char *doc_type_name, const char *doc_name );
};

class ASViewManager : public ASView
{
        /* this is static data that defines applicatrion at compile time and should not be deallocated */
        struct ASHashTable *m_ViewTypes ;             /* hashed by name */

        /* this structures are allocated in run time and as such should be deallocated on exit */
        vector<ASView*>  m_Views ;                      /* array of pointers to open documents */
    public :
        ASViewManager(ASDocManager *doc_manager);
        ~ASViewManager();       

        /* ASView virtual methods : */
        Bool    Open();
        void    Refresh();
        void    Close();
        Bool    HandleXEvent(struct ASEvent *event);

        
        /* VIEW manager specific stuff : */
        Bool    RegisterViewType( ASViewType *type );
        ASViewType*  LookupViewType( const char *view_type_name );

        ASView*  CreateNewView( const char *view_type_name, ASDoc *doc );
};

class ASDocViewFramework : public ASDoc
{    
        vector<ASDocViewTemplate*> m_Templates ;                 /* array of pointers to ASDocViewTemplate objects */
    
    public :

        ASDocManager    *m_DocManager ;
        ASViewManager   *m_ViewManager ;
        
    public :

        ASDocViewFramework( const char *app_name ) : ASDoc( app_name )
            { m_DocManager = new ASDocManager; m_ViewManager = new ASViewManager(m_DocManager);}; 
        ~ASDocViewFramework();

        /* ASDoc virtual methods : */
        Bool    OpenFile( const char *filename ) { return False; };    
        Bool    SaveFile( const char *filename = NULL ){ return False; };    
        Bool    CheckSanity(){ return True; };
        
        /* doc and view types are added only if specific code ( say GTK ) is compiled in*/
        Bool    RegisterDocType( ASDocType *type ){ return m_DocManager->RegisterDocType( type );};     
        Bool    RegisterViewType( ASViewType *type ){ return m_ViewManager->RegisterViewType( type );};
        /* templates are added for any possible combination of doc and view type, regardless if
         * specific code is compiled in or not. It gets linked to spoecific object 
         * at the run-time by looking it up in hash tables :
         */
        Bool    AddDocViewTemplate( const char *doc_type_name, const char *view_type_name );
        
        ASDocType*  LookupDocType( const char *doc_type_name ){ return m_DocManager->LookupDocType( doc_type_name ); };
        ASViewType*  LookupViewType( const char *view_type_name ){ return m_ViewManager->LookupViewType( view_type_name ); };

        Bool OpenMainFrame() { return m_ViewManager->Open(); };
        Bool HandleXEvent(ASEvent *event)  { return m_ViewManager->HandleXEvent(event); };

        Bool InstatiateTemplate( ASDocViewTemplate *tmpl, ASDoc *doc = NULL );

};

#ifdef __cplusplus
extern "C" {
#endif
/* any potential c code should go here , althou I do not anticipate any */
#ifdef __cplusplus
}
#endif


#endif

