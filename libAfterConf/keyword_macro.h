#ifndef KEYWORD_MACRO_H_FILE_INCLUDED
#define KEYWORD_MACRO_H_FILE_INCLUDED


#define ASCF_DEFINE_MODULE_FLAG_XREF(module,keyword,struct_type) \
	{module##_##keyword,module##_##keyword##_ID,0,offsetof(struct_type,flags),offsetof(struct_type,set_flags)}	
#define ASCF_DEFINE_MODULE_ONOFF_FLAG_XREF(module,keyword,struct_type) \
	{module##_##keyword,module##_##keyword##_ID,module##_No##keyword##_ID,offsetof(struct_type,flags),offsetof(struct_type,set_flags)}	


#define ASCF_DEFINE_KEYWORD(module,flags,keyword,type,subsyntax) \
	{(flags),#keyword,sizeof(#keyword)-1,type,module##_##keyword##_ID,subsyntax,0,0,0}	

#define ASCF_DEFINE_KEYWORD_S(module,flags,keyword,type,subsyntax,struct_type) \
	{(flags),#keyword,sizeof(#keyword)-1,type,module##_##keyword##_ID,subsyntax,offsetof(struct_type,keyword),module##_##keyword,0}	

#define ASCF_DEFINE_KEYWORD_SA(module,flags,keyword,type,subsyntax,struct_type,alias_for) \
	{(flags),#keyword,sizeof(#keyword)-1,type,module##_##keyword##_ID,subsyntax,offsetof(struct_type,alias_for),module##_##alias_for,0}	

#define ASCF_DEFINE_KEYWORD_SF(module,flags,keyword,type,subsyntax,struct_type,flags_on,flags_off) \
	{(flags),#keyword,sizeof(#keyword)-1,type,module##_##keyword##_ID,subsyntax,offsetof(struct_type,keyword),flags_on,flags_off}	

#define ASCF_DEFINE_KEYWORD_F(module,flags,keyword,subsyntax,flags_on,flags_off) \
	{(flags),#keyword,sizeof(#keyword)-1,TT_FLAG,module##_##keyword##_ID,subsyntax,0,flags_on,flags_off}	


#define ASCF_PRINT_FLAG_KEYWORD(stream,module,__config,keyword) \
	do{ if(get_flags((__config)->set_flags,module##_##keyword) ) \
			fprintf (stream, #module "." #keyword " = %s;\n",get_flags((__config)->flags,module##_##keyword)? "On" : "Off");}while(0)

#define ASCF_PRINT_FLAGS_KEYWORD(stream,module,__config,keyword) \
	do{ if(get_flags((__config)->set_flags,module##_##keyword) ) fprintf (stream, "%s : " #module "." #keyword " = 0x%lX;\n", MyName, (__config)->keyword); }while(0)

#define ASCF_PRINT_GEOMETRY_KEYWORD(stream,module,__config,keyword) \
	do{ if(get_flags((__config)->set_flags,module##_##keyword) )	\
		{ 	fprintf (stream, #module "." #keyword " = "); \
			if(get_flags((__config)->keyword.flags,WidthValue))	fprintf (stream, "%u",(__config)->keyword.width); \
			fputc( 'x', stream ); \
			if(get_flags((__config)->keyword.flags,HeightValue))	fprintf (stream, "%u",(__config)->keyword.height); \
			if(get_flags((__config)->keyword.flags,XValue))	fprintf (stream, "%+d",(__config)->keyword.x); \
			else if(get_flags((__config)->keyword.flags,YValue)) fputc ('+',stream);	\
			if(get_flags((__config)->keyword.flags,XValue))	fprintf (stream, "%+d",(__config)->keyword.y); \
			fputc( ';', stream ); fputc( '\n', stream ); \
		}}while(0)

#define ASCF_PRINT_SIZE_KEYWORD(stream,module,__config,keyword) \
	do{ if(get_flags((__config)->set_flags,module##_##keyword) )	\
			fprintf (stream, #module "." #keyword " = %ux%u;\n",(__config)->keyword.width,(__config)->keyword.height); }while(0)

#define ASCF_PRINT_INT_KEYWORD(stream,module,__config,keyword) \
	do{ if(get_flags((__config)->set_flags,module##_##keyword) ) fprintf (stream, #module "." #keyword " = %d;\n",(__config)->keyword); }while(0)

#define ASCF_PRINT_STRING_KEYWORD(stream,module,__config,keyword) \
	do{ if( (__config)->keyword != NULL  ) fprintf (stream, #module "." #keyword " = \"%s\";\n",(__config)->keyword); }while(0)

#define ASCF_PRINT_IDX_STRING_KEYWORD(stream,module,__config,keyword,idx) \
	do{ if( (__config)->keyword[idx] != NULL  ) fprintf (stream, #module "." #keyword "[%d] = \"%s\";\n",idx,(__config)->keyword[idx]); }while(0)

#define ASCF_PRINT_COMPOUND_STRING_KEYWORD(stream,module,__config,keyword,separator) \
	do{ if( (__config)->keyword != NULL  ) \
		{int __i__; \
			fprintf (stream, #module "." #keyword " = \""); \
			for( __i__=0; (__config)->keyword[__i__]!=NULL ; ++__i__ ) \
				fprintf (stream, "%s%c",(__config)->keyword[__i__],separator); \
			fputc( ';', stream ); fputc( '\n', stream ); \
		}}while(0)

#define ASCF_PRINT_IDX_COMPOUND_STRING_KEYWORD(stream,module,__config,keyword,separator,idx) \
	do{ if( (__config)->keyword[idx] != NULL  ) \
		{int __i__; \
			fprintf (stream, #module "." #keyword "[%d] = \"",idx); \
			for( __i__=0; (__config)->keyword[idx][__i__]!=NULL ; ++__i__ ) \
				fprintf (stream, "%s%c",(__config)->keyword[idx][__i__],separator); \
			fputc( '\"', stream ); fputc( ';', stream ); fputc( '\n', stream ); \
		}}while(0)

#define ASCF_HANDLE_SUBSYNTAX_KEYWORD_CASE(module,__config,__curr,keyword,type) \
		case module##_##keyword##_##ID : \
			if( __curr->sub ){ \
				set_flags(__config->set_flags,module##_##keyword); \
		   		__config->keyword = Parse##type##Options( __curr->sub ); }break

#define ASCF_HANDLE_ALIGN_KEYWORD_CASE(module,__config,__curr,keyword) \
		ASCF_HANDLE_SUBSYNTAX_KEYWORD_CASE(module,__config,__curr,keyword,Align)

#define ASCF_HANDLE_BEVEL_KEYWORD_CASE(module,__config,__curr,keyword) \
		ASCF_HANDLE_SUBSYNTAX_KEYWORD_CASE(module,__config,__curr,keyword,Bevel)
			

#define ASCF_HANDLE_GEOMETRY_KEYWORD_CASE(module,__config,__item,keyword) \
		case module##_##keyword##_##ID : set_flags(__config->set_flags,module##_##keyword); \
			__config->keyword = __item.data.geometry; break

#define ASCF_HANDLE_SIZE_KEYWORD_CASE(module,__config,__item,keyword) \
		case module##_##keyword##_##ID : set_flags(__config->set_flags,module##_##keyword); \
			__config->keyword.width = get_flags( __item.data.geometry.flags, WidthValue )?__item.data.geometry.width:0; \
			__config->keyword.height = get_flags( __item.data.geometry.flags, HeightValue )?__item.data.geometry.height:0; \
			break
			
#define ASCF_HANDLE_INTEGER_KEYWORD_CASE(module,__config,__item,keyword) \
		case module##_##keyword##_##ID : set_flags(__config->set_flags,module##_##keyword); \
			__config->keyword = __item.data.integer; break

#define ASCF_HANDLE_STRING_KEYWORD_CASE(module,__config,__item,keyword) \
		case module##_##keyword##_##ID : \
			if( __config->keyword ) free( __config->keyword ); \
			__config->keyword = __item.data.string; break


#define ASCF_MERGE_FLAGS(__to,__from) \
    do{	(__to)->flags = ((__from)->flags&(__from)->set_flags)|((__to)->flags & (~(__from)->set_flags)); \
    	(__to)->set_flags |= (__from)->set_flags;}while(0)

#define ASCF_MERGE_GEOMETRY_KEYWORD(module,__to,__from,keyword) \
	do{ if( get_flags((__from)->set_flags, module##_##keyword) )	\
        	merge_geometry(&((__from)->keyword), &((__to)->keyword) ); }while(0)

#define ASCF_MERGE_SCALAR_KEYWORD(module,__to,__from,keyword) \
	do{ if( get_flags((__from)->set_flags, module##_##keyword) )	\
        	(__to)->keyword = (__from)->keyword ; }while(0)

#define ASCF_MERGE_STRING_KEYWORD(module,__to,__from,keyword) \
	do{ if( (__from)->keyword )	\
        {	set_string(	&((__to)->keyword), stripcpy2((__from)->keyword, False));}}while(0)


#endif /* KEYWORD_MACRO_H_FILE_INCLUDED */
