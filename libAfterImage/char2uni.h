#ifndef CHAR2UNI_H_HEADER_INCLUDED
#define CHAR2UNI_H_HEADER_INCLUDED

typedef enum {
   CHARSET_ISO8859_1 = 0,     /* Does not require translation */
   CHARSET_ISO8859_2, 
   CHARSET_ISO8859_3, 
   CHARSET_ISO8859_4, 
   CHARSET_ISO8859_5, 
   CHARSET_ISO8859_6, 
   CHARSET_ISO8859_7, 
   CHARSET_ISO8859_8, 
   CHARSET_ISO8859_9, 
   CHARSET_ISO8859_10,
   CHARSET_ISO8859_13,
   CHARSET_ISO8859_14,
   CHARSET_ISO8859_15,
   CHARSET_ISO8859_16,
   CHARSET_KOI8_R, 
   CHARSET_KOI8_RU,
   CHARSET_KOI8_U, 
   CHARSET_CP1250, 
   CHARSET_CP1251, 
   CHARSET_CP1252,
   SUPPORTED_CHARSETS_NUM
}ASSupportedCharsets ;

extern const unsigned short *as_current_charset ;

#ifdef WIN32 
#define UNICODE_CHAR	CARD16
#else
#define UNICODE_CHAR	CARD32
#endif

/*
#define CHAR2UNICODE(c) \
  ((UNICODE_CHAR)(((((unsigned short)(c))&0x0080)!=0)? \
					as_current_charset[((unsigned short)text[i])&0x007F]:\
			       ((unsigned char)(c))))
*/
#ifdef  I18N
#define CHAR2UNICODE(c) \
  ((UNICODE_CHAR)(((((unsigned short)(c))&0x0080)!=0)? \
					as_current_charset[((unsigned short)(c))&0x007F]:\
			       ((unsigned char)(c))))
#else
#define CHAR2UNICODE(c)   ((UNICODE_CHAR)(((unsigned short)(c))&0x00FF))
#endif

ASSupportedCharsets as_set_charset( ASSupportedCharsets new_charset );
ASSupportedCharsets parse_charset_name( const char *name );


/****d* libAfterImage/CHAR_SIZE
 * FUNCTION
 * Convinient macro so we can transparently determine the number of
 * bytes that character spans. It assumes UTF-8 encoding when I18N is
 * enabled.
 * SOURCE
 */
/* size of the UTF-8 encoded character is based on value of the first byte : */
#define UTF8_CHAR_SIZE(c) 	(((c)&0x80)?(((c)&0x40)?(((c)&0x20)?(((c)&0x10)?5:4):3):2):1)
#ifdef WIN32
#define UNICODE_CHAR_SIZE(c)	sizeof(UNICODE_CHAR)
#endif
#define CHAR_SIZE(c) 			1
/*************/


#endif