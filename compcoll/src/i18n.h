/**
 * @file    i18n.h
 * @brief   convertion and charset detection functions
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2009-04-13
 */
#ifndef __MY_I18N_H
#define __MY_I18N_H

/*#include <locale.h> // setlocale() */
#include <stdlib.h>

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG
#define TRACE(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define DBGMSG(catlog, level, fmt, ...) fprintf (stderr, "[%s()]\t" fmt "\t{%d," __FILE__ "}\n", __func__, ##__VA_ARGS__, __LINE__)
#else
#define TRACE(...)
#define DBGMSG(...)
#endif

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#if USE_ICONV
#include <iconv.h>
#include <chardetect.h>

#else
#define CHARDET_MAX_ENCODING_NAME 30
typedef void * chardet_t;
extern int my_chardet_create(chardet_t* pdet);
extern void my_chardet_destroy(chardet_t det);
extern int my_chardet_handle_data(chardet_t det, const char* data, unsigned int len);
extern int my_chardet_data_end(chardet_t det);
extern int my_chardet_get_charset(chardet_t det, char* namebuf, unsigned int buflen);
#define chardet_create my_chardet_create
#define chardet_destroy my_chardet_destroy
#define chardet_handle_data my_chardet_handle_data
#define chardet_data_end my_chardet_data_end
#define chardet_get_charset my_chardet_get_charset

typedef void *iconv_t;
extern iconv_t my_iconv_open (const char *__tocode, const char *__fromcode);
extern int my_iconv_close (iconv_t __cd);
extern size_t my_iconv (iconv_t __cd, char ** __inbuf,
                     size_t * __inbytesleft,
                     char ** __outbuf,
                     size_t * __outbytesleft);
#define iconv_open my_iconv_open
#define iconv_close my_iconv_close
#define iconv my_iconv

#endif

/*
#define ENCTYP_UNKNOWN 0x00
#define ENCTYP_UTF8    0x01
#define ENCTYP_UTF16LE 0x02
#define ENCTYP_UTF16BE 0x03
#define ENCTYP_UTF32LE 0x04
#define ENCTYP_UTF32BE 0x05
#define ENCTYP_GB18030 0x06
#define ENCTYP_GB2312  0x07
#define ENCTYP_BIG5    0x08
int chardet_cstr2val (char * encname);
*/

#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /* __MY_I18N_H */
