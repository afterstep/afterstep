#ifndef CHAR2UNI_H_HEADER_INCLUDED
#define CHAR2UNI_H_HEADER_INCLUDED

enum {
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

extern unsigned short *current_ascharset ;

#define CHAR2UNICODE(c) \
  ((CARD16)(c)<128 || (CARD16)(c)>=256 )?\
   (CARD16)(c):\
   current_ascharset[(CARD16)(c)-128])

ASSupportedCharsets switch_ascharset( ASSupportedCharsets new_charset );

#endif