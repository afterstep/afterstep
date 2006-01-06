/*
 * Copyright (c) 2005 Sasha Vasko <sasha at aftercode.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include "../../../configure.h"
#include "../../../libAfterStep/asapp.h"
#include "../../../libAfterStep/module.h"
#include "../../../libAfterStep/session.h"
#include "../../../libAfterConf/afterconf.h"
#ifdef HAVE_GTK
#include "../../../libASGTK/asgtk.h"
#else
typedef void* GtkWidget;
#endif
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#define ENTRY_WIDTH		300


typedef enum ASRunTool
{
	ASRTool_Normal = 0,
	ASRTool_Term,
	ASRTool_Browser,
	ASRTool_Editor,
	ASRTool_KDEScreenSaver,
	ASRTool_ASConfigFile,
	ASRTool_Tools
}ASRunTool;

typedef struct ASRunState
{
#define ASRUN_Persist 			(0x01<<1)
#define ASRUN_Immidiate			(0x01<<2)
	
	ASFlagType flags ;
	
	ASRunTool  tool ;

	GtkWidget *main_window ;	   
	
	GtkWidget *target_combo ;
	GtkWidget *target_entry ;
	GtkWidget *run_in_term_check ;

	Window kde_screensaver_window ;
}ASRunState;

ASRunState AppState ;

#if 0
char *get_default_web_browser()
{
	static char *known_browsers[] = { "x-www-browser",
									"firefox",
									"mozilla-firefox",
									"mozilla",
									"opera", 
									"lynx",
									NULL };
	int i ;							
	for( i = 0 ; known_browsers[i] ; ++i ) 
		if( is_executable_in_path (known_browsers[i]) )
			return mystrdup(known_browsers[i]);
	return NULL;
}
#endif
Bool create_KDEScreenSaver_window()			   
{
	Window        w;
	XSizeHints    shints;
	ExtendedWMHints extwm_hints ;
    XSetWindowAttributes attr;

	if( AppState.kde_screensaver_window != None ) 
		return True;

    attr.background_pixmap = None ;
	attr.event_mask = ButtonPressMask|PointerMotionMask|KeyPressMask ;
    w = create_visual_window( Scr.asv, Scr.Root, 0, 0, Scr.MyDisplayWidth, Scr.MyDisplayHeight, 0, InputOutput, CWEventMask|CWBackPixmap, &attr);
	if( w == None ) 
		return False;
    set_client_names( w, "KDEscreensaver.kss", "KDEscreensaver.kss", "KDEscreensaver", "KDEscreensaver" );

	memset( &shints, 0x00, sizeof(shints));    
    shints.flags = PSize|PPosition ;
	
	extwm_hints.pid = getpid();
    extwm_hints.flags = EXTWM_PID|EXTWM_StateSkipTaskbar|EXTWM_StateSkipPager|EXTWM_StateFullscreen ;

	set_client_hints( w, NULL, &shints, AS_DoesWmDeleteWindow, &extwm_hints );

	/* showing window to let user see that we are doing something */
	XMapRaised (dpy, w);
    /* final cleanup */
	XFlush (dpy);
	sleep_a_millisec(500);
	/* we have to give AS a chance to spot us */

	AppState.kde_screensaver_window = w ;

	return (w!= None) ;
}	 

Bool
exec_command(char **ptext, ASRunTool tool)
{
	char *text = *ptext ;
	if( text[0] == '\0' ) 
	{
		free( *ptext ); 
		*ptext = NULL ;
		text = NULL ;	  
	}	 
	if( text != NULL && tool == ASRTool_Normal )
	{
		if( mystrncasecmp(text, "http://", 7) == 0 ||
			mystrncasecmp(text, "https://", 8) == 0 ||
			mystrncasecmp(text, "ftp://", 6) == 0 )
		{
			tool = ASRTool_Browser ;
		}else
		{
			if( !is_executable_in_path( text ) ) 	
		 		if( CheckFile( text ) == 0 )
					tool = ASRTool_Editor ;		
		}	 
	}
	if( text && tool == ASRTool_KDEScreenSaver ) 
	{
		if( !is_executable_in_path( text ) ) 	  
			return False;
		else
		{
			char *tmp ; 
			if( !create_KDEScreenSaver_window()	)
				return False;
			tmp = safemalloc( strlen(text)+1+256 );
			sprintf( tmp, "%s -window-id %ld", text, AppState.kde_screensaver_window );
			free( text ) ;
			text = *ptext = tmp ;
		}		  
	}	 
	if( text && tool == ASRTool_ASConfigFile ) 		
	{
		if( isalpha(text[0]) ) 
		{
			char *fullfilename = NULL ; 
			char *src = NULL ;
			const char *const_src = NULL ;
			if( mystrcasecmp( text, "look" ) == 0 ) 
			{
				fullfilename = make_session_data_file(Session, False, 0, LOOK_DIR, DEFAULT_USER_LOOK, NULL );
				const_src = get_session_file (Session, 0, F_CHANGE_LOOK, False);
			}else if( mystrncasecmp( text, "look.", 5 ) == 0 ) 
			{
				fullfilename = make_session_data_file(Session, False, 0, LOOK_DIR, text, NULL );
				const_src = get_session_file (Session, 0, F_CHANGE_LOOK, False);
			}else if( mystrcasecmp( text, "feel" ) == 0 ) 
			{
				fullfilename = make_session_data_file(Session, False, 0, FEEL_DIR, DEFAULT_USER_FEEL, NULL );
				const_src = get_session_file (Session, 0, F_CHANGE_FEEL, False);
			}else if( mystrncasecmp( text, "feel.", 5 ) == 0 ) 
			{
				fullfilename = make_session_data_file(Session, False, 0, FEEL_DIR, text, NULL );
				const_src = get_session_file (Session, 0, F_CHANGE_FEEL, False);
			}else
			{
				fullfilename = make_session_data_file(Session, False, 0, text, NULL );
				src = make_session_data_file(Session, True, 0, text, NULL );
			}	 
			if( CheckFile(fullfilename) != 0 ) 
				if( src || const_src ) 
				{
					if( CopyFile (src?src:const_src, fullfilename) != 0 )
					{
						if( src ) 
							free( src );
						free( fullfilename );
						return False;
					}		   
					if( const_src ) 
					{
						SendTextCommand ( F_QUICKRESTART, NULL, "startmenu", 0);
					}
				}	 
			if( src ) 
				free( src );
			free( text ) ;
			*ptext = text = fullfilename ;
		}	 
	}
	if( text )
	{	
		FunctionCode func = F_NOP ;
		switch(tool)
		{
			case ASRTool_Normal : 	func = F_EXEC ; break;
			case ASRTool_Term : 	func = F_ExecInTerm ; break;
			case ASRTool_Browser : 	func = F_ExecBrowser ; break;
			case ASRTool_Editor : 	func = F_ExecEditor ; break;
			case ASRTool_KDEScreenSaver : 	func = F_EXEC ; break;
			case ASRTool_ASConfigFile : 	func = F_ExecEditor ; break;
			default: break;
		}	 
		if( func != F_NOP )
		{	
			SendTextCommand ( func, NULL, text, 0);
			sleep_a_millisec(500);
		}
		return True;
	}
	return False ;
}

#ifdef HAVE_GTK
void
on_destroy(GtkWidget *widget, gpointer user_data)
{
	gtk_main_quit();
}



void
on_exec_clicked(GtkWidget *widget, gpointer user_data)
{
	if( AppState.target_entry )
	{	
		Bool in_term = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(AppState.run_in_term_check) );
		char *text = stripcpy(gtk_entry_get_text(GTK_ENTRY(AppState.target_entry)));

		if( exec_command(&text, in_term?ASRTool_Term:ASRTool_Normal) )
		{
			if( !get_flags( AppState.flags, ASRUN_Persist ) )
				gtk_main_quit();
			else
				asgtk_combo_box_add_to_history( GTK_COMBO_BOX(AppState.target_combo), text );
			
			free( text );
		}
	}
}

void init_ASRun(ASFlagType flags, ASRunTool tool, const char *cmd )
{
	
	GtkWidget *main_vbox ;
	GtkWidget *hbox ;
	GtkWidget *frame ;
	GtkWidget *exec_btn, *cancel_btn ;

	memset( &AppState, 0x00, sizeof(AppState));
	AppState.flags = flags ;
	AppState.tool = tool ;

	AppState.main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  	gtk_window_set_title (GTK_WINDOW (AppState.main_window), MyName);
	/*gtk_window_set_resizable( GTK_WINDOW (AppState.main_window), FALSE);*/
	colorize_gtk_widget( AppState.main_window, get_colorschemed_style_normal() );

	frame = gtk_frame_new( NULL );
	gtk_container_add (GTK_CONTAINER (AppState.main_window), frame);
	gtk_container_set_border_width( GTK_CONTAINER (frame), 5 );
	gtk_widget_show(frame);

	main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), main_vbox);
	gtk_container_set_border_width( GTK_CONTAINER (main_vbox), 5 );

	
	/********   Line 1 *******/
	AppState.run_in_term_check = gtk_check_button_new_with_label("Exec in terminal");
	if( AppState.tool == ASRTool_Term )
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(AppState.run_in_term_check), TRUE );

	hbox = gtk_hbox_new( FALSE, 5 );
	gtk_box_pack_start (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new("Command to execute:"), FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), AppState.run_in_term_check, FALSE, FALSE, 0);
	gtk_widget_show_all (hbox);
	
	/********   Line 2 *******/
	AppState.target_combo = gtk_combo_box_entry_new_text(); 
	gtk_container_set_border_width( GTK_CONTAINER (AppState.target_combo), 1 );
	gtk_widget_set_size_request (AppState.target_combo, ENTRY_WIDTH, -1);
	
	frame = gtk_frame_new( NULL );
	gtk_container_add (GTK_CONTAINER (frame), AppState.target_combo);
	gtk_container_set_border_width( GTK_CONTAINER (frame), 1 );
	gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 5);
	gtk_widget_show_all(frame);
	gtk_widget_show(frame);
	colorize_gtk_edit( AppState.target_combo );
	
	if( GTK_IS_CONTAINER(AppState.target_combo) )
		gtk_container_forall( GTK_CONTAINER(AppState.target_combo), find_combobox_entry, &AppState.target_entry );
	
	/********   Line 3 *******/
	hbox = gtk_hbutton_box_new();
	gtk_box_pack_end (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 5);
	exec_btn = gtk_button_new_from_stock( GTK_STOCK_EXECUTE );
	gtk_box_pack_start (GTK_BOX (hbox), exec_btn, FALSE, FALSE, 0);
	cancel_btn = gtk_button_new_from_stock( GTK_STOCK_CANCEL );
	gtk_box_pack_end (GTK_BOX (hbox), cancel_btn, FALSE, FALSE, 0);
	gtk_widget_show_all (hbox);
	   	
	/********   Callbacks *******/
	/* if above succeeded then path_entry should be not NULL here : */
	/* TODO : insert proper change handlers and data pointers here : */
	if( AppState.target_entry ) 
	{	
		gtk_entry_set_has_frame(  GTK_ENTRY(AppState.target_entry), FALSE );
		g_signal_connect ( G_OBJECT (AppState.target_entry), "activate",
		      			   G_CALLBACK (on_exec_clicked), (gpointer) NULL);
		if( cmd )
			gtk_entry_set_text( GTK_ENTRY(AppState.target_entry), cmd );
	}

	g_signal_connect (G_OBJECT(AppState.target_combo), "changed",
			  			G_CALLBACK (NULL), (gpointer) NULL);
	
	g_signal_connect (G_OBJECT (cancel_btn), "clicked", G_CALLBACK (on_destroy), NULL);
	g_signal_connect (G_OBJECT (exec_btn), "clicked", G_CALLBACK (on_exec_clicked), NULL);
	
	g_signal_connect (G_OBJECT (AppState.main_window), "destroy", G_CALLBACK (on_destroy), NULL);
	/********   Show them all *******/
	gtk_widget_show_all (main_vbox);

	gtk_widget_show(AppState.main_window);
}	 
#endif                         /* HAVE_GTK  */

int
main (int argc, char *argv[])
{
	ASFlagType flags = 0 ; 
	ASRunTool tool = ASRTool_Normal ;
	int i;
	char * initial_command = NULL ;

#ifdef HAVE_GTK		   
	init_asgtkapp( argc, argv, CLASS_ASRUN, NULL, 0);
#else
	InitMyApp (CLASS_ASRUN, argc, argv, NULL, NULL, 0 );
  	LinkAfterStepConfig();
  	InitSession();
    ConnectX( ASDefaultScr, 0 );
	LoadColorScheme();
	ReloadASEnvironment( NULL, NULL, NULL, False, True );
	set_flags( flags, ASRUN_Immidiate );
#endif	
	
	for( i = 1 ; i < argc ; ++i ) 
	{	
		if( argv[i] == NULL ) 
			continue;
		if( mystrcasecmp( argv[i], "--exec-in-term" ) == 0 )
			tool = ASRTool_Term;
		else if( mystrcasecmp( argv[i], "--open-in-browser" ) == 0 )
			tool = ASRTool_Browser;
		else if( mystrcasecmp( argv[i], "--open-in-editor" ) == 0 )
			tool = ASRTool_Editor;
		else if( mystrcasecmp( argv[i], "--KDE-screensaver" ) == 0 )
		{
			tool = ASRTool_KDEScreenSaver;
			++i ;
			if( argv[i] != NULL  ) 
			{
				initial_command = mystrdup(argv[i]);
				tool = ASRTool_KDEScreenSaver;
				set_flags( flags, ASRUN_Immidiate );
			}
		}else if( mystrcasecmp( argv[i], "--afterstep-config" ) == 0 )
		{
			++i ;
			if( argv[i] != NULL  ) 
			{
				initial_command = mystrdup(argv[i]);
				tool = ASRTool_ASConfigFile;
				set_flags( flags, ASRUN_Immidiate );
			}
		}else if( mystrcasecmp( argv[i], "--persist" ) == 0 )
			set_flags( flags, ASRUN_Persist );
		else if( mystrcasecmp( argv[i], "--immidiate" ) == 0 )
			set_flags( flags, ASRUN_Immidiate );
		else if( mystrcasecmp( argv[i], "--cmd" ) == 0 && argv[i+1] != NULL )
		{
			++i ;
			initial_command = mystrdup(argv[i]);
		}
	}
	
	ConnectAfterStep(0,0);
	
	if( get_flags( flags, ASRUN_Immidiate ) && initial_command != NULL )
	{
		memset( &AppState, 0x00, sizeof(AppState));
		AppState.flags = flags ;
		AppState.tool = tool ;
		exec_command(&initial_command, tool);
		if( tool == ASRTool_KDEScreenSaver && AppState.kde_screensaver_window != None ) 
		{
			XEvent dum;	
			do
			{
			 	XNextEvent( dpy, &dum );
			 	fprintf( stderr, "event %d\n", dum.type );
			}while(dum.type != KeyPress && dum.type != ButtonPress && dum.type != MotionNotify);
			XDestroyWindow( dpy, AppState.kde_screensaver_window );
		}	 
	}else
	{
#ifdef HAVE_GTK		
		init_ASRun( flags, tool, initial_command );
  		gtk_main ();
#endif
	}
  	return 0;
}

