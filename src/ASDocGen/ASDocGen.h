#ifndef ASDOCGEN_H_HEADER_INCLUDED
#define ASDOCGEN_H_HEADER_INCLUDED


#define OVERVIEW_SIZE_THRESHOLD 1024

typedef enum { 
	DocType_Plain = 0,
	DocType_HTML,
	DocType_PHP,
	DocType_XML,
	DocType_NROFF,
	DocType_Source,
	DocTypes_Count
}ASDocType;

extern const char *PHPXrefFormat;
extern const char *PHPXrefFormatSetSrc;
extern const char *PHPXrefFormatUseSrc;
extern const char *PHPCurrPageFormat;

extern const char *AfterStepName;
extern const char *APIGlossaryName; 
extern const char *APITopicIndexName ; 
extern const char *GlossaryName; 
extern const char *TopicIndexName; 

extern const char *ASDocTypeExtentions[DocTypes_Count];

typedef enum { 
	DocClass_Overview = 0,
	DocClass_BaseConfig,
	DocClass_MyStyles,
	DocClass_Options,
	DocClass_TopicIndex,
	DocClass_Glossary
}ASDocClass;

extern const char *StandardOptionsEntry;
extern const char *MyStylesOptionsEntry;
extern const char *BaseOptionsEntry;

extern const char *DocClassStrings[4][2];

#define DOC_CLASS_Overview   	(0x01<<DocClass_Overview)
#define DOC_CLASS_BaseConfig   	(0x01<<DocClass_BaseConfig)
#define DOC_CLASS_MyStyles      (0x01<<DocClass_MyStyles)
#define DOC_CLASS_Options       (0x01<<DocClass_Options)
#define DOC_CLASS_None          (0xFFFFFF00)


typedef struct ASXMLInterpreterState {

#define ASXMLI_LiteralLayout		(0x01<<0)	  
#define ASXMLI_InsideLink			(0x01<<1)	  
#define ASXMLI_FirstArg 			(0x01<<2)	  
#define ASXMLI_LinkIsURL			(0x01<<3)	  
#define ASXMLI_LinkIsLocal			(0x01<<4)	  
#define ASXMLI_InsideExample		(0x01<<5)	  
#define ASXMLI_ProcessingOptions	(0x01<<6)	  
#define ASXMLI_RefSection			(0x01<<7)	  
#define ASXMLI_FormalPara			(0x01<<8)	  
#define ASXMLI_EscapeDQuotes		(0x01<<9)	  

	
	ASFlagType flags;

	const char *doc_name ;   
	const char *display_name ;
	const char *display_purpose ;
	
	FILE *dest_fp ;
	char *dest_file ;
	const char *dest_dir ;
	ASDocType doc_type ;
	int header_depth ;
	int group_depth ;
	char *curr_url_page ;
	char *curr_url_anchor ;

	int pre_options_size ;

	ASFlagType doc_class_mask ;
	ASDocClass doc_class ;

}ASXMLInterpreterState ;

extern const char *HTML_CSS_File ;
extern const char *CurrHtmlBackFile ;
#define DATE_SIZE 64
extern char CurrentDateLong[DATE_SIZE];
extern char CurrentDateShort[DATE_SIZE];

extern ASHashTable *DocBookVocabulary ;

extern ASHashTable *ProcessedSyntaxes ;
extern ASHashTable *Glossary ;
extern ASHashTable *Index ;
extern ASHashTable *Links ;
extern int DocGenerationPass ;
extern int CurrentManType ;

#endif /*ASDOCGEN_H_HEADER_INCLUDED*/

