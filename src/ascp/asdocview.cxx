/*
 * Copyright (c) 2003 Sasha Vasko <sasha@aftercode.net>
 *
 * This module is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

/*#define DO_CLOCKING      */
#define LOCAL_DEBUG



#include "../../configure.h"
#include "../../libAfterStep/asapp.h"
#include "../../libAfterStep/event.h"

#include "asdocview.h"

/************************** ASDocType ************************************/

ASDocType:: ASDocType( const char *name )
{
    m_Name = mystrdup( name );
    m_Magic = XInternAtom( dpy, m_Name, False );    
}    

ASDocType:: ~ASDocType()
{
    free( m_Name );    
}     

/************************** ASDoc ****************************************/
ASDoc :: ASDoc( const char *name )
{
    m_Name = mystrdup(name);    
    m_Filename = NULL ; 
    m_Dirty = False ;
}    

ASDoc:: ~ASDoc()
{
    free( m_Name );
    if( m_Filename ) 
        free( m_Filename );
}                    

void
ASDoc:: AttachView( ASView *view )
{
    if( view ) 
    {
        m_Views.push_back( view );    
    }        
}

void
ASDoc:: DetachView( ASView *view )
{
    if( view ) 
    {
        vector<ASView*>::iterator i = m_Views.begin(); 

        for( i = m_Views.begin() ; i != m_Views.end() ; ++i )        
            if( *i == view ) 
            {
                m_Views.erase( i );
                break;
            }
    }        
}

void 
ASDoc:: RefreshAllViews()
{
    for( unsigned int i = 0 ; i < m_Views.size() ; ++i ) 
        if( m_Views[i] ) 
            m_Views[i]->Refresh();    
}

/************************** ASViewType ***********************************/

ASViewType:: ASViewType( const char *name )
{
    m_Name = mystrdup( name );
    m_Magic = XInternAtom( dpy, m_Name, False );    
}    

ASViewType :: ~ASViewType()
{
    free( m_Name );    
}    

/************************** ASView ***************************************/
ASView:: ASView( ASDoc *doc, ASView *parent )
{
    m_Doc = doc ; 
    m_Parent = parent ;
    m_Window = None ;      

    m_Open = False ;

    if( doc ) 
        doc->AttachView( this );
}    

ASView::~ASView()
{
    if( m_Open ) 
        Close();
    if( m_Doc ) 
        m_Doc->DetachView( this );    
}    

/************************* ASDocViewTemplate *****************************/
ASDocViewTemplate::ASDocViewTemplate( const char *doc_type, const char *view_type )
{
    m_DocTypeName = mystrdup( doc_type );
    m_ViewTypeName = mystrdup( view_type );
}
    
ASDocViewTemplate::~ASDocViewTemplate()
{
    free( m_DocTypeName );
    free( m_ViewTypeName );    
}    

/************************* ASDocManager      *****************************/
void destroy_ASDocType (ASHashableValue value, void *data)
{
    ASDocType *doc_type = static_cast<ASDocType*>(data);    
    if( doc_type )
        delete doc_type ;
}    
 
               
ASDocManager::ASDocManager():ASDoc( "Document Manager" )
{
    m_DocTypes = create_ashash( 7, string_hash_value, string_compare, destroy_ASDocType );
}    

ASDocManager::~ASDocManager()
{
    if( m_DocTypes ) 
        destroy_ashash( &m_DocTypes );
    for( unsigned int i = 0 ; i < m_Docs.size() ; ++i )
        delete m_Docs[i] ;
}    

/* Doc manager specific stuff : */
Bool    
ASDocManager::RegisterDocType( ASDocType *type )
{
    if( type == NULL ) 
        return False;    

    return (add_hash_item( m_DocTypes, AS_HASHABLE(type->m_Name), (void*)type ) == ASH_Success );
}    

ASDocType*  
ASDocManager::LookupDocType( const char *doc_type_name )
{
    ASHashData hdata = {0} ;
    if( get_hash_item( m_DocTypes, AS_HASHABLE(doc_type_name), &hdata.vptr ) != ASH_Success )
        return NULL;
    return static_cast<ASDocType*>(hdata.vptr);
}    

ASDoc*  
ASDocManager::CreateNewDoc(  const char *doc_type_name, const char *doc_name )
{
    ASDocType *type = LookupDocType( doc_type_name );
    if( type ) 
    {
        ASDoc *doc = type->CreateDoc( doc_name );
        if( doc ) 
        {
            m_Docs.push_back( doc );    
            return doc ;
        }    
    }
    return NULL;
}

/************************* ASViewManager     *****************************/
void destroy_ASViewType (ASHashableValue value, void *data)
{
    ASViewType *v_type = static_cast<ASViewType*>(data);    
    if( v_type )
        delete v_type ;
}    
 
ASViewManager::ASViewManager(ASDocManager *doc_manager)
    :ASView( doc_manager, NULL )
{
    m_ViewTypes = create_ashash( 7, string_hash_value, string_compare, destroy_ASViewType );
}    

ASViewManager::~ASViewManager()
{
    if( m_ViewTypes ) 
        destroy_ashash( &m_ViewTypes );
    for( unsigned int i = 0 ; i < m_Views.size() ; ++i )
        delete m_Views[i] ;
}    

Bool    
ASViewManager::Open()
{
    
    return True;    
}

void    
ASViewManager::Refresh()
{
    
    
}

void    
ASViewManager::Close()
{
}

Bool    
ASViewManager::HandleXEvent(struct ASEvent *event)
{
    for( unsigned int i = 0 ; i < m_Views.size() ; ++i )
        if( m_Views[i]->WindowXID() == event->w ) 
            return m_Views[i]->HandleXEvent( event );
    /* we need to handle some CLIENT events here ? */
    return False;
}

 
/* VIEW manager specific stuff : */
Bool    
ASViewManager::RegisterViewType( ASViewType *type )
{
    if( type == NULL ) 
        return False;    

    return (add_hash_item( m_ViewTypes, AS_HASHABLE(type->m_Name), (void*)type ) == ASH_Success );
}    

ASViewType*  
ASViewManager::LookupViewType( const char *view_type_name )
{
    ASHashData hdata = {0} ;
    if( get_hash_item( m_ViewTypes, AS_HASHABLE(view_type_name), &hdata.vptr ) != ASH_Success )
        return NULL;
    return static_cast<ASViewType*>(hdata.vptr);
}    

ASView*  
ASViewManager::CreateNewView( const char *view_type_name, ASDoc *doc )
{
    ASViewType *type = LookupViewType( view_type_name );
    ASView *view ;
    if( type ) 
    {    
        if( (view = type->CreateView( doc, this )) != NULL ) 
        {
            m_Views.push_back( view );
            return view;            
        }
    }        
    return NULL;
}

/************************* ASDocViewFramework *****************************/

ASDocViewFramework::~ASDocViewFramework()
{
    delete m_ViewManager ; 
    delete m_DocManager ; 

    for( unsigned int i = 0 ; i < m_Templates.size() ; ++i )
        delete m_Templates[i] ;
}


Bool    
ASDocViewFramework:: AddDocViewTemplate( const char *doc_type_name, const char *view_type_name )
{
    ASDocViewTemplate *t = new ASDocViewTemplate( doc_type_name, view_type_name );
    m_Templates.push_back( t );

    return True;
}

Bool 
ASDocViewFramework:: InstatiateTemplate( ASDocViewTemplate *tmpl, ASDoc *doc )
{
    ASView    *view = NULL ;

    if( tmpl == NULL ) 
        return False;
    
    if( LookupViewType( tmpl->GetViewTypeName() ) == NULL ) 
        return False;
    
    if( doc == NULL ) 
    {    
        doc = m_DocManager->CreateNewDoc( tmpl->GetDocTypeName(), NULL );
        if( doc == NULL ) 
            return False ;
    }
    
    view = m_ViewManager->CreateNewView( tmpl->GetViewTypeName(), doc );
    return ( view != NULL );
}


/**
 * Main loop should look something like that : 
 * 
 * while( 1 ) 
 * {
 *      while( XPendingEvent() ) 
 *      {
 * #ifdef GTK 
 *          if( gtk_events_pending() )
 *              gtk_main_iteration();
 *          else
 * #endif
 *          {
 *              XNextEvent(dpy, &xevent );
 * #ifdef FLTK
 *              if( !Fl:fl_handle( &xevent ) )
 * #endif
 *              {
 *                  ASEvent event ;
 *                  DigestEvent( &xevent, event );
 *                  DocViewFramework->HandleXEvent( event );
 *              }    
 *          }
 *      }
 *      wait for pipes
 * }
 */


