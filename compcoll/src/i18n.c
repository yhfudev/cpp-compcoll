/**
 * @file    i18n.c
 * @brief   convertion and charset detection functions
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2009-04-13
 */
/* 支持 ICU Libary (International Components for Unicode, developed by IBM) */
#if USE_ICONV
#else

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <unicode/ucnv.h>
#include <unicode/ucnv_err.h>
#include <unicode/ucsdet.h>

//#include "pfall.h"
//#include "sdict_type.h"
#include "i18n.h"

int
my_chardet_create (chardet_t* pdet)
{
    UErrorCode status = U_ZERO_ERROR;
    UCharsetDetector *csd;
    if (NULL == pdet) {
        return -1;
    }
    csd = ucsdet_open (&status);
    assert (NULL != pdet);
    *pdet = (chardet_t)csd;
    return 0;
}

void
my_chardet_destroy (chardet_t det)
{
    ucsdet_close ((UCharsetDetector *)det);
}

int
my_chardet_handle_data (chardet_t det, const char* data, unsigned int len)
{
    UErrorCode status = U_ZERO_ERROR;
    UCharsetDetector *csd = (UCharsetDetector *)det;
    ucsdet_setText (csd, data, len, &status);
    if (U_FAILURE(status)) {
        return -1;
    }
    return 0;
}

int
my_chardet_data_end (chardet_t det)
{
    return 0;
}

int
my_chardet_get_charset (chardet_t det, char* namebuf, unsigned int buflen)
{
    UErrorCode status = U_ZERO_ERROR;
    const char *name;
    const UCharsetMatch *match;
    UCharsetDetector *csd = (UCharsetDetector *)det;
    match = ucsdet_detect (csd, &status);
    if (match == NULL) {
        return -1;
    }
    name = ucsdet_getName (match, &status);
    if (NULL == name) {
        return -1;
    }
    if (strlen (name) > buflen - 1) {
        return -1;
    }
    strcpy (namebuf, name);
    return 0;
}

/************************************************************************/
typedef struct _iconv_item_t {
    UConverter * targetCnv;
    UConverter * sourceCnv;
} iconv_item_t;

iconv_t
my_iconv_open (const char *__tocode, const char *__fromcode)
{
    /* ./source/test/cintltst/ccapitst.c:2065: ucnv_convert */
    UErrorCode status = U_ZERO_ERROR;
    iconv_item_t *reticonv;

    assert (NULL != __tocode);
    assert (NULL != __fromcode);

    reticonv = (iconv_item_t *) malloc (sizeof (*reticonv));
    if (NULL == reticonv) {
        return NULL;
    }
    reticonv->targetCnv = ucnv_open(__tocode, &status);
    if (U_FAILURE(status)) {
        DBGMSG (PFDBG_CATLOG_USR_UTILS, PFDBG_LEVEL_ERROR, "Error in ucnv_open('%s') 1!\n", __tocode);
        DBGMSG (PFDBG_CATLOG_USR_UTILS, PFDBG_LEVEL_ERROR, "Please refer to the Charset Alias: http://www.unicode.org/reports/tr22/\n");
        free (reticonv);
        assert (0);
        return NULL;
    }
    reticonv->sourceCnv = ucnv_open(__fromcode, &status);
    if (U_FAILURE(status)) {
        DBGMSG (PFDBG_CATLOG_USR_UTILS, PFDBG_LEVEL_ERROR, "Error in ucnv_open('%s') 2!\n", __fromcode);
        DBGMSG (PFDBG_CATLOG_USR_UTILS, PFDBG_LEVEL_ERROR, "Please refer to the Charset Alias: http://www.unicode.org/reports/tr22/\n");
        ucnv_close (reticonv->targetCnv);
        free (reticonv);
        assert (0);
        return NULL;
    }
    return (iconv_t)reticonv;
}

int
my_iconv_close (iconv_t __cd)
{
    iconv_item_t *reticonv = (iconv_item_t *)__cd;

    assert (NULL != reticonv);
    ucnv_close (reticonv->targetCnv);
    ucnv_close (reticonv->sourceCnv);
    free (reticonv);
    return 0;
}

size_t
my_iconv (iconv_t __cd, char ** __inbuf, size_t * __inbytesleft, char ** __outbuf, size_t * __outbytesleft)
{
    UErrorCode status = U_ZERO_ERROR;
    char *target;
    const char *source;
    iconv_item_t *reticonv = (iconv_item_t *)__cd;

    assert (NULL != __cd);
    assert (NULL != __inbuf);
    assert (NULL != *__inbuf);
    assert (NULL != __outbuf);
    assert (NULL != *__outbuf);
    assert (NULL != __inbytesleft);
    assert (NULL != __outbytesleft);

    target = *__outbuf;
    source = (const char *)(*__inbuf);
    reticonv = (iconv_item_t *)__cd;
    assert (NULL != reticonv->targetCnv);
    assert (NULL != reticonv->sourceCnv);

    ucnv_convertEx (reticonv->targetCnv, reticonv->sourceCnv, &target, target + *__outbytesleft,
        &source, source + *__inbytesleft, NULL, NULL, NULL, NULL, TRUE, TRUE, &status);
    if (U_FAILURE(status)) {
        DBGMSG (PFDBG_CATLOG_USR_UTILS, PFDBG_LEVEL_WARNING, "Some error in ucnv_convertEx()\n");
        /*return 0; */
    }
    *__inbytesleft -= (source - *__inbuf);
    *__inbuf = (char *)source;
    *__outbytesleft -= (target - *__outbuf);
    *__outbuf = target;

    return (target - *__outbuf);
}
#endif

/*
#define CHKRET_NAME(name,typ)    if (0 == strcasecmp (encname, name)) return typ;
int
chardet_cstr2val (char * encname)
{
    CHKRET_NAME ("utf-8",    ENCTYP_UTF8);
    CHKRET_NAME ("UTF-16LE", ENCTYP_UTF16LE);
    CHKRET_NAME ("UTF-16BE", ENCTYP_UTF16BE);
    CHKRET_NAME ("UTF-32LE", ENCTYP_UTF32LE);
    CHKRET_NAME ("UTF-32BE", ENCTYP_UTF32BE);
    CHKRET_NAME ("GB18030",  ENCTYP_GB18030);
    CHKRET_NAME ("GB2312",   ENCTYP_GB2312);
    CHKRET_NAME ("BIG5",     ENCTYP_BIG5);
    return "ISO8859-1"
}
*/
