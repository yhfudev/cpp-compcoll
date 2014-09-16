/**
 * @file    octstring.c
 * @brief   C 的可伸缩字串结构, 实现类似于C++语言中的 String
 * 包括预留缓存以供将来添加数据的优化
 * 支持二进制数据存储
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2006/02/24
 * @copyright GNU Lesser General Public License v2.0.
 */
#include <stdlib.h>             /* malloc() */
#include <string.h>             /* memset() */
#include <ctype.h>              /* isprint() */
#include <stdarg.h>
#include <errno.h>
#include <assert.h>


//#if defined(WIN32)
//#include "win32.h"
//#include "gettimeofday.h"
//#else
//#include <netinet/in.h>         /* htons(), htonl() */
//#include <sys/time.h>           /* gettimeofday() */
//#endif

#include "octstring.h"

#define WCHAR_BYTE_WIDTH 4 /*sizeof (wchar_t)*/
#define OSTR_ZEROTAIL(buffer, pos) memset (((uint8_t *)buffer) + pos, 0, WCHAR_BYTE_WIDTH)

#define OSTR_HAVNT_ENOUGHSPACE(currentlen, wantlen) ((currentlen) <= (wantlen) + WCHAR_BYTE_WIDTH)

/*
#ifdef __cplusplus
extern "C" {
#endif
*/

#ifndef _DEBUG
/*#define _DEBUG 1*/
#endif

#ifndef ostr_assert
#ifdef _DEBUG
#define ostr_assert(a) if (!(a)) {assert (a);}
#else
#define ostr_assert(a)
#endif
#endif /* ostr_assert */

/*#define OSTR_TRACE DS_TRACE*/
#define OSTR_TRACE(a)

#if defined(_DEBUG) /* && (0 != DEBUG)*/
#define GET_BULK_SIZE(init_size, result) \
do { \
if(init_size >= SIZE_ATOM) \
{ \
   result = init_size / 2; \
   if(result < SIZE_ATOM) \
     result = SIZE_ATOM; \
   result += init_size; \
} \
else \
  result = SIZE_ATOM; \
} while(0)
#else
#define GET_BULK_SIZE(init_size, result)
#endif

/*#define SIZE_SYS_MEM (256 * 1024 * 1024) // 256M */
#define SIZE_SYS_MEM (8 * 1024 * 1024)  /* 8M */

#if defined(DEBUG) && (0 != DEBUG)
#define SIZE_ATOM 40
#else
#define SIZE_ATOM 40
#endif
#define SIZE_MORE_MAX (SIZE_SYS_MEM / 1024)
#if (SIZE_MORE_MAX < SIZE_ATOM)
#undef SIZE_MORE_MAX
#define SIZE_MORE_MAX SIZE_ATOM
#endif
#define COUNT_REALLOC_MAX (2 - 1)

#define GET_BULK_SIZE_MORE(osp, init_all_len, result) \
do { \
if(init_all_len > SIZE_ATOM) \
{ \
   result = (osp->size_request_more + init_all_len - osp->size_buf) / (osp->count_request + 1); \
   if(osp->count_request > COUNT_REALLOC_MAX) \
   { \
     size_t other_more = 0; \
     other_more = (init_all_len) - osp->size_buf; \
     other_more <<= 1; \
     if(other_more > SIZE_MORE_MAX) \
       other_more = SIZE_MORE_MAX; \
     if(other_more > (result)) \
       result = other_more; \
   } \
   if((result) < SIZE_ATOM) \
     result = SIZE_ATOM; \
   result += (init_all_len); \
} \
else \
  result = SIZE_ATOM; \
} while(0)


#define CHK_OSTR_RETURN(osp, ret_on_fail) do{ if (NULL == (osp)) return (ret_on_fail); }while(0)

/*#define SECU_MEMSET(p, v, l) wtls_secu_memset((p),(v),(l)) */
#define SECU_MEMSET(p, v, l) memset((p),(v),(l))


/* - - - - Stat for octstr - - - - - */
#define TEST_OCTSTR 0

#if TEST_OCTSTR
#include "hashtit.h"
/*
#define gw_malloc malloc
#define gw_free free
*/

#define ostrmem_realloc(p, size, filename, line) realloc(p, size)
#define ostrmem_free(p, filename, line)          free(p)
#define ostrmem_strdup(p, filename, line)        strdup(p)

#define ostrmem_malloc(s, filename, line) ((call_malloc) ++, size_all += (s), malloc(s))
#define ostrmem_memmove(target, source, len) ((call_memmove ++), (size_memmove += (len)), memmove((target), (source), (len)))

#define OSTR_ADD_UNUSED(ostr) (size_unused += ostr_left_space(ostr))
size_t call_malloc = 0;
size_t call_memmove = 0;
size_t size_memmove = 0;
size_t size_all = 0;
size_t size_unused = 0;


#define STAT_INIT_REAL() (((ht_stat = ht_create(3, &hkfunc_charstr, 0, NULL)) == NULL)?-1:0)
#define STAT_DESTROY_REAL() \
( \
DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "call_malloc = %ld", (long)call_malloc), \
DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "call_memmove = %ld", (long)call_memmove), \
DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "size_memmove = %ld", (long)size_memmove), \
DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "size_all = %ld", (long)size_all), \
DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "size_unused = %ld", (long)size_unused), \
ostr_stat_dump(), ht_destroy(ht_stat), ht_stat = NULL, 0)

#define TRACE_FUNC(func_name) \
do { \
  int count = 0; \
  ht_del(ht_stat, (hashkey_t)(#func_name), (void **)(&count)); \
  count ++; \
  ht_add(ht_stat, #func_name, (void *)count); \
} while(0)

hashtable_t *ht_stat = 0;

int
ostr_stat_dump (void)
{
    hasht_iter_t *hi;
    void *next = NULL;
    int count;
    char *str;
    int i;

    hi = htiter_create (ht_stat);
    if (0 == hi)
    {
        DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "error in HashtIter_Create()");
        return (-1);
    }
    next = htiter_begin (hi);
    count = (int) htiter_userdata (next);
    for (i = htiter_length (hi); i > 0; i--)
    {
        count = (int) htiter_userdata (next);
        str = (char *) htiter_key (next);
        DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "\t%s\t= %ld", str, (long) count);
        next = htiter_next (hi);
    }
    htiter_destroy (hi);
    hi = 0;
    return 0;
}

/*
#define STAT_INIT STAT_INIT_REAL
#define STAT_DESTROY STAT_DESTROY_REAL
*/
int
STAT_INIT (void)
{
    return STAT_INIT_REAL ();
}

int
STAT_DESTROY (void)
{
    DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "call_malloc = %ld", (long)call_malloc);
    DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "call_memmove = %ld", (long)call_memmove);
    DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "size_memmove = %ld", (long)size_memmove);
    DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "size_all = %ld", (long)size_all);
    DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "size_unused = %ld", (long)size_unused);
    ostr_stat_dump(); ht_destroy(ht_stat); ht_stat = NULL;
    return 0;
    /*return STAT_DESTROY_REAL ();*/
}

#else

#if MEMWATCH
    #define ostrmem_realloc(p, size, filename, line)    mwRealloc(p, size, filename, line)
    #define ostrmem_free(p, filename, line)             mwFree(p, filename, line)
    #define ostrmem_strdup(p, filename, line)           mwStrdup(p, filename, line)
    #define ostrmem_malloc(s, filename, line)           mwMalloc(s, filename, line)
#else
    #define ostrmem_realloc(p, size, filename, line) realloc(p, size)
    #define ostrmem_free(p, filename, line) free(p)
    #define ostrmem_strdup(p, filename, line) strdup(p)
    #define ostrmem_malloc(s, filename, line) malloc(s)
#endif

#define ostrmem_memmove(target, source, len) memmove((target), (source), (len))
#define OSTR_ADD_UNUSED(ostr)

/*
#define STAT_INIT()
#define STAT_DESTROY()
*/
int STAT_INIT (void);
int STAT_DESTROY (void);
int ostr_stat_dump (void);

int
STAT_INIT (void)
{
    return 0;
}

int
STAT_DESTROY (void)
{
    return 0;
}

#define TRACE_FUNC(func_name)
/*#define ostr_stat_dump() (0)*/
int
ostr_stat_dump (void)
{
    return 0;
}
#endif
/* - - - - - - - - - - - - - - - - - */

//static int ostr_destroy_secu (ostr_t * osp);
static ssize_t ostr_cb_fsys_read_fd (void *fd, void * buffer, size_t size);
static ssize_t ostr_cb_fsys_write_fd (void *fd, void * buffer, size_t size);
static ssize_t ostr_cb_fsys_read_stream (void *fp, void * buffer, size_t size);
static ssize_t ostr_cb_fsys_write_stream (void *fp, void * buffer, size_t size);
static ssize_t ostr_format_valist (ostr_t * osp, size_t offset, const char *fmt, va_list args);
static int bhd_cb_writer_null (void *fd, opaque_t * fragment, size_t size);
static int _mem_d2b_writer (void * fd, opaque_t * fragment, size_t size);
static int _ostr_d2b_writer (void * fd, opaque_t * fragment, size_t size);

#if OSTRMEM_TRACE
int
ostr_init_dbg (ostr_t * osp, size_t size, const char * filename, int line)
#else
int
ostr_init (ostr_t * osp, size_t size)
#endif
{
    if (NULL == osp) {
        DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "parameter: osp");
        return -1;
    }

    memset ((void *) osp, 0, sizeof (ostr_t));

#ifdef _DEBUG
    osp->flag_dyn_alloc = 0;
#endif /*_DEBUG */
    osp->buf = NULL;
    osp->size_buf = 0;
    osp->size_data = 0;

    osp->size_request_more = 0;
    osp->count_request = 0;
    osp->count_copy = 0;

    if (size < 1) /* size == 0 */
    {
        return 0;
    }
    return ostr_require_space_dbg (osp, size, filename, line);
}

int
ostr_init_from_bulk (ostr_t * osp, opaque_t * bulk, size_t size)
{
    int ret;

    if (NULL == osp)
        return -1;

    ret = ostr_init (osp, size);
    if (ret != 0)
        return ret;
    if (size > 0) {
        assert (! OSTR_HAVNT_ENOUGHSPACE (osp->size_buf, size));
        ostrmem_memmove (osp->buf, bulk, size);
        /*osp->buf[size] = '\0'; */
        OSTR_ZEROTAIL (osp->buf, size);
        osp->size_data = size;
    }
    return 0;
}

/**
 * @brief 创建 ostr_t
 *
 * @param size : 预留缓冲大小
 *
 * @return 成功返回一个新的 ostr_t 指针
 *
 * 创建 ostr_t
 */
#if OSTRMEM_TRACE
ostr_t *
ostr_create_dbg (size_t size, const char *filename, int line)
#else
ostr_t *
ostr_create (size_t size)
#endif
{
    ostr_t *osp = NULL;

    TRACE_FUNC (ostr_create);
    osp = (ostr_t *)ostrmem_malloc (sizeof (ostr_t), filename, line);
    if (NULL == osp)
        return NULL;

    if (0 != ostr_init (osp, size))
    {
        ostrmem_free (osp, filename, line);
        return NULL;
    }
#ifdef _DEBUG
    osp->flag_dyn_alloc = 1;
#endif /* _DEBUG */
    OSTR_CHK_INTEGRITY (osp);
    return osp;
}

/**
 * @brief 销毁 ostr_t
 *
 * @param osp : ostr_t 指针
 *
 * @return 成功返回0
 *
 * 销毁 ostr_t
 */
int
ostr_destroy (ostr_t * osp)
{
    TRACE_FUNC (ostr_destroy);

    if (NULL == osp)
        return 0;
    ostr_assert (NULL != osp);
    OSTR_CHK_INTEGRITY (osp);

    /*#if defined(DEBUG) || TEST_OCTSTR */
#if 0
    DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "ostr_dump_to_stream(osp):\n");
    ostr_dump_to_stream (osp, stdout);
#endif
    if (NULL != osp->buf)
    {
        OSTR_ADD_UNUSED (osp);
#if TEST_OCTSTR
        SECU_MEMSET (osp->buf, 0, osp->size_data);
#endif
        ostrmem_free (osp->buf, __FILE__, __LINE__);
    }

#ifdef _DEBUG
    ostr_assert (1 == osp->flag_dyn_alloc);
#endif /* _DEBUG */
    /*if (0 != osp->flag_dyn_alloc) {*/
        ostrmem_free (osp, __FILE__, __LINE__);
    /*} */
    return 0;
}

#if 0
static int
ostr_destroy_secu (ostr_t * osp)
{
    ostr_assert (NULL != osp);
    OSTR_CHK_INTEGRITY (osp);

    /*#if defined(DEBUG) || TEST_OCTSTR */
#if 0
    DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "ostr_dump_to_stream(osp):\n");
    ostr_dump_to_stream (osp, stdout);
#endif
    if (NULL != osp->buf)
    {
        SECU_MEMSET (osp->buf, 0, osp->size_data);
        ostrmem_free (osp->buf, __FILE__, __LINE__);
    }
#ifdef _DEBUG
    ostr_assert (1 == osp->flag_dyn_alloc);
#endif /* _DEBUG */
    /*if (0 != osp->flag_dyn_alloc) { */
        ostrmem_free (osp, __FILE__, __LINE__);
    /*} */
    return 0;
}
#endif

/**
 * @brief 以bulk中的内容来创建 ostr_t
 *
 * @param bulk : 需复制内容的C缓冲
 * @param size : C缓冲大小
 *
 * @return 成功返回一个新的 ostr_t 指针
 *
 * 以bulk中的内容来创建 ostr_t
 */
ostr_t *
ostr_create_from_bulk (opaque_t * bulk, size_t size)
{
    ostr_t *osp = NULL;

    TRACE_FUNC (ostr_create_from_bulk);
    ostr_assert (NULL != bulk);
    ostr_assert (0 <= size);
    /*
    CHK_OSTR_RETURN (bulk, NULL);
    if (size < 0)
        return NULL;
    */

    osp = ostr_create (size);
    if (NULL == osp)
        return NULL;
    if (NULL == osp->buf) {
        ostr_require_space (osp, size);
    }
    if (NULL == osp->buf) {
        ostr_destroy (osp);
        osp = NULL;
    }
    else if (size > 0)
    {
        ostrmem_memmove (osp->buf, bulk, size);
        osp->count_copy++;
    }
    /*osp->buf[size] = 0; */
    OSTR_ZEROTAIL (osp->buf, size);

    osp->size_data = size;

    OSTR_CHK_INTEGRITY (osp);
    return osp;
}

/**
 * @brief 获取 ostr_t 当前数据大小
 *
 * @param osp : ostr_t 指针
 *
 * @return 成功返回ostr_t 当前数据大小
 *
 * 获取 ostr_t 当前数据大小
 */
ssize_t
ostr_length (ostr_t * osp)
{
    if (NULL == osp) {
        return -1;
    }
    return osp->size_data;
}

/**
 * @brief 获取 ostr_t 当前可添加数据大小
 *
 * @param osp : ostr_t 指针
 *
 * @return 成功返回ostr_t 当前可添加数据大小
 *
 * 获取 ostr_t 当前在重请求新缓冲前可添加数据大小
 */
ssize_t
ostr_left_space (ostr_t * osp)
{
    if (NULL == osp) {
        return -1;
    }
    return (osp->size_buf - osp->size_data);
}

/**
 * @brief 获取 ostr_t 当前内部缓冲大小
 *
 * @param osp : ostr_t 指针
 *
 * @return 成功返回ostr_t 当前内部缓冲大小
 *
 * 获取 ostr_t 当前内部缓冲大小
 */
ssize_t
ostr_total_space (ostr_t * osp)
{
    if (NULL == osp) {
        return -1;
    }
    return (osp->size_buf);
}

ssize_t
ostr_append_from_bulk (ostr_t * osp, size_t len, opaque_t * bulk)
{
    if (NULL == osp) {
        return -1;
    }
    return ostr_copy_from_bulk ((osp), ostr_length(osp), (len), (opaque_t *)(bulk));
}

ssize_t
ostr_append_from_peer (ostr_t * osp, size_t len, ostr_t * peer, size_t peer_offset)
{
    if (NULL == osp) {
        return -1;
    }
    return ostr_copy_from_peer ((osp), ostr_length(osp), (len), (peer), (peer_offset));
}

opaque_t *
ostr_getref_data(ostr_t * osp, size_t offset, size_t len)
{
    if (ostr_length (osp) < 0) {
        return NULL;
    }
    if (offset + len > (size_t)ostr_length (osp)) {
        return NULL;
    }
    return (osp->buf + offset);
}

ssize_t
ostr_dump_to_stream (ostr_t * osp, size_t max_size, FILE *fp)
{
    if (NULL == osp) {
        return -1;
    }
    return ostr_dump_to_fd ((osp), (max_size), fileno(fp));
}

#if OSTRMEM_TRACE
int
ostr_require_space_dbg (ostr_t * osp, size_t max_size, const char *filename, int line)
#else
int
ostr_require_space (ostr_t * osp, size_t max_size)
#endif
{
    size_t real_size;
    opaque_t *newbuf;

    TRACE_FUNC (ostr_require_space);
    ostr_assert (NULL != osp);
    CHK_OSTR_RETURN (osp, -1);
    /*OSTR_CHK_INTEGRITY (osp);*/

    if (! OSTR_HAVNT_ENOUGHSPACE (osp->size_buf, max_size))
        return 0;
    max_size += WCHAR_BYTE_WIDTH; /*4: 为了添加字符串的结尾 "0", 对于 Unicode 的结束符是 2-4个 0 */

    GET_BULK_SIZE_MORE (osp, max_size, real_size);

    /* DEBUG */
    if (real_size > 100*1024*1024) {
        /*DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "alloc more than 100MB: realsize=%"PRIuSZ", maxsize=%"PRIuSZ")", real_size, max_size);*/
        DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "alloc more than 100MB: realsize=%"PRIuSZ", maxsize=%"PRIuSZ")\n", real_size, max_size);
        assert (0);
        return -1;
    }

    /*DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "old size: %d, new size: %d", osp->size_buf, real_size);*/
#if 0
    newbuf = (opaque_t *)ostrmem_realloc (osp->buf, real_size, filename, line);
    if (NULL == newbuf)
    {
        DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "ralloc(buf=0x%08X, sz=%d, sz_old=%d)", osp->buf, real_size, osp->size_buf);
        return -1;
    }
#else /* 1 */
    newbuf = (opaque_t *)ostrmem_malloc (real_size, filename, line);
    if (NULL == newbuf)
    {
        DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "malloc()");
        return -1;
    }

    if (NULL != osp->buf)
    {
        if (osp->size_data > 0) {
            ostrmem_memmove (newbuf, osp->buf, osp->size_data);
#if TEST_OCTSTR
            SECU_MEMSET (osp->buf, 0, osp->size_data);
#endif
        }
        ostrmem_free (osp->buf, filename, line);
    }
    else
    {
        osp->size_data = 0;
    }
#endif /* 1 */

    /*newbuf[osp->size_data] = 0; */
    OSTR_ZEROTAIL (newbuf, osp->size_data);
    osp->buf = newbuf;

    osp->size_request_more += (real_size - osp->size_buf);
    osp->count_request++;

    osp->size_buf = real_size;

    OSTR_CHK_INTEGRITY (osp);
    return 0;
}

/* 只是对osp中的buffer进行写操作，如果范围超过当前长度，则改写size_data到超过的地方 */
#if OSTRMEM_TRACE
ssize_t
ostr_copy_from_bulk_dbg (ostr_t * osp, size_t offset, size_t len, opaque_t * bulk, const char *filename, int line)
#else
ssize_t
ostr_copy_from_bulk (ostr_t * osp, size_t offset, size_t len, opaque_t * bulk)
#endif
{
    size_t size_all;

    TRACE_FUNC (ostr_copy_from_bulk);
    ostr_assert (NULL != osp);  /* CHK_OSTR_RETURN (bulk, -1);*/
    ostr_assert (NULL != bulk); /* CHK_OSTR_RETURN (osp, -1);*/
    OSTR_CHK_INTEGRITY (osp);

    if (len < 1) {
        return 0;
    }

    if ((NULL == osp->buf) || (OSTR_HAVNT_ENOUGHSPACE (osp->size_buf, offset + len)))
    {
        if (0 > ostr_require_space_dbg (osp, offset + len + 1, filename, line))
        {
            DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "ostr_require_space()");
            return -1;
        }
    }

    if (offset > osp->size_data)
    {
        memset ((void *) (((opaque_t *) (osp->buf)) + osp->size_data), 0, (size_t) (offset + 1 - osp->size_data));
    }

    size_all = offset + len;
    if (size_all > osp->size_data)
    {
        osp->size_data = size_all;
        OSTR_ZEROTAIL (osp->buf, osp->size_data);
    }

    ostrmem_memmove (((opaque_t *) (osp->buf)) + offset, bulk, len);

    assert (osp->size_data + 2 <= osp->size_buf);
    osp->buf[osp->size_data] = 0;
    osp->buf[osp->size_data + 1] = 0;

    osp->count_copy++;

    OSTR_CHK_INTEGRITY (osp);
    return len;
}

ssize_t
ostr_copy_to_bulk (ostr_t * osp, size_t offset, size_t len, opaque_t * bulk)
{
    TRACE_FUNC (ostr_copy_to_bulk);
    ostr_assert (NULL != osp);
    ostr_assert (NULL != bulk);
    CHK_OSTR_RETURN (bulk, -1);
    CHK_OSTR_RETURN (osp, -1);
    OSTR_CHK_INTEGRITY (osp);

    if (offset > osp->size_data)
        return -1;

    if (len > osp->size_data - offset) {
        len = osp->size_data - offset;
    }

    if (len > 0)
    {
        ostrmem_memmove (bulk, (void *) (((unsigned char *) (osp->buf)) + offset), len);
    }

    OSTR_CHK_INTEGRITY (osp);
    return len;
}

#if OSTRMEM_TRACE
ssize_t
ostr_copy_from_peer_dbg (ostr_t * osp, size_t offset, size_t len, ostr_t * peer, size_t peer_offset, const char *filename, int line)
#else
ssize_t
ostr_copy_from_peer (ostr_t * osp, size_t offset, size_t len, ostr_t * peer, size_t peer_offset)
#endif
{
    TRACE_FUNC (ostr_copy_from_peer);
    ostr_assert (NULL != osp);
    ostr_assert (NULL != peer);
    CHK_OSTR_RETURN (osp, -1);
    CHK_OSTR_RETURN (peer, -1);
    OSTR_CHK_INTEGRITY (osp);

    if (len < 1) {
        return 0;
    }
    if (ostr_length (peer) < 1) {
        return 0;
    }
    if (peer->size_data <= peer_offset) {
        return 0;
    }
    if (peer->size_data < peer_offset + len) {
        assert (peer->size_data > peer_offset);
        len = peer->size_data - peer_offset;
    }

    return ostr_copy_from_bulk_dbg (osp, offset, len, peer->buf + peer_offset, filename, line);
}

/**
 * @brief 将一个ostr_t中的数据挪到对方
 *
 * @param osp_to : 内容的新位置
 * @param osp_from : 要转移的内容
 *
 * @return 成功返回0
 *
 * 将一个ostr_t中的数据挪到对方， buf 被置换到 osp_to, 原来的ostr_t变空
 */
int
ostr_transfer (ostr_t * osp_to, ostr_t * osp_from)
{
#ifdef _DEBUG
    int is_dyn = 0;

    is_dyn = osp_to->flag_dyn_alloc;
#endif /* _DEBUG */

    ostr_clear (osp_to);
    ostr_init (osp_to, 0);
    if (osp_to->buf) {
        ostrmem_free (osp_to->buf, __FILE__, __LINE__);
    }
    ostrmem_memmove (osp_to, osp_from, sizeof (*osp_to));

#ifdef _DEBUG
    osp_to->flag_dyn_alloc = is_dyn;
    is_dyn = osp_from->flag_dyn_alloc;
#endif /* _DEBUG */
    memset (osp_from, 0, sizeof (*osp_from));
    ostr_init (osp_from, 0);
#ifdef _DEBUG
    osp_from->flag_dyn_alloc = is_dyn;
#endif /* _DEBUG */
    return 0;
}

/**
 * @brief 将bulk中指定位置的数据插入到 ostr_t 指定位置
 *
 * @param osp : ostr_t 指针
 * @param offset : 要插入的起始位置
 * @param len : bulk数据长度
 * @param bulk : C缓冲
 *
 * @return 成功返回插入的数据长度
 *
 * 将bulk中指定位置的数据插入到 ostr_t 指定位置
 */
ssize_t
ostr_insert_from_bulk (ostr_t * osp, size_t offset, size_t len, opaque_t * bulk)
{
    size_t size_all;

    TRACE_FUNC (ostr_insert_from_bulk);
    ostr_assert (NULL != osp);
    ostr_assert (NULL != bulk);
    CHK_OSTR_RETURN (bulk, -1);
    CHK_OSTR_RETURN (osp, -1);
    OSTR_CHK_INTEGRITY (osp);

    if (len < 1)
        return 0;

    if (offset > osp->size_data)
        size_all = offset + len;
    else
        size_all = osp->size_data + len;

    if (0 > ostr_require_space (osp, size_all + 1))
        return -1;

    if (offset > osp->size_data)
    {
        memset ((void *) (((opaque_t *) (osp->buf)) + osp->size_data), 0, offset - osp->size_data + 1);
    }
    else if (offset < osp->size_data)
    {
        ostrmem_memmove (((opaque_t *) (osp->buf)) + offset + len,
                      ((opaque_t *) (osp->buf)) + offset, osp->size_data - offset);
    }

    ostrmem_memmove (((opaque_t *) (osp->buf)) + offset, bulk, len);
    osp->count_copy++;

    osp->buf[size_all] = 0;
    OSTR_ZEROTAIL (osp->buf, size_all);
    osp->size_data = size_all;

    OSTR_CHK_INTEGRITY (osp);
    return len;
}

/**
 * @brief 将对方ostr_t中指定位置的数据插入到 ostr_t 指定位置
 *
 * @param osp : ostr_t 指针
 * @param offset : 要插入的起始位置
 * @param len : 数据长度
 * @param peer : 对方ostr_t
 * @param peer_offset : 对方数据的起始位置
 *
 * @return 成功返回插入的数据长度
 *
 * 将对方ostr_t中指定位置的数据插入到 ostr_t 指定位置
 */
ssize_t
ostr_insert_from_peer (ostr_t * osp, size_t offset, size_t len, ostr_t * peer, size_t peer_offset)
{
    ssize_t real_len;

    TRACE_FUNC (ostr_insert_from_peer);
    ostr_assert (NULL != osp);
    ostr_assert (NULL != peer);
    CHK_OSTR_RETURN (osp, -1);
    CHK_OSTR_RETURN (peer, -1);
    OSTR_CHK_INTEGRITY (osp);

    if (peer->size_data <= peer_offset) {
        return -1;
    }
    real_len = peer->size_data;
    real_len -= (ssize_t) peer_offset;
    if (real_len > (ssize_t) len) {
        real_len = (ssize_t) len;
    }

    OSTR_CHK_INTEGRITY (osp);
    return ostr_insert_from_bulk (osp, offset, real_len, peer->buf + peer_offset);
}

int
ostr_clear (ostr_t * osp)
{
    if (NULL != osp->buf) {
        ostrmem_free (osp->buf, __FILE__, __LINE__);
    }
    memset (osp, 0, sizeof (*osp));
    return 0;
}

void
ostr_free4reserve (ostr_t * osp, size_t newlen)
{
    if (newlen < osp->size_buf / 2) {
        ostrmem_free (osp->buf, __FILE__, __LINE__);
        osp->buf = NULL;
        osp->size_buf = 0;
        osp->size_data = 0;
    }
}

/**
 * @brief 将 ostr_t 当前数据截断成指定大小
 *
 * @param osp : ostr_t 指针
 * @param newlen : 新的数据长度
 *
 * @return 成功返回ostr_t 新的数据长度
 *
 * 将 ostr_t 当前数据截断成指定大小
 */
ssize_t
ostr_truncate (ostr_t * osp, size_t newlen)
{
    TRACE_FUNC (ostr_truncate);
    ostr_assert (NULL != osp);
    CHK_OSTR_RETURN (osp, -1);
    OSTR_CHK_INTEGRITY (osp);

    assert (newlen < 1024*1024*128);
    if (newlen < 1) /* newlen == 0 */
    {
        osp->size_data = 0;
        /*return ostr_clear (osp); */
        if (NULL != osp->buf)
        {
#if 0
            ostrmem_free (osp->buf, __FILE__, __LINE__);
            osp->buf = NULL;
            osp->size_buf = 0;
            osp->size_data = 0;
#else
            /*osp->buf[0] = 0; */
            if (osp->size_buf > 0) {
                assert (osp->size_buf > 0);
                memset (osp->buf, 0, osp->size_buf);
            }
#endif
        }
        return 0;
    }

    if (OSTR_HAVNT_ENOUGHSPACE (osp->size_buf, newlen)) {
        if (0 > ostr_require_space (osp, newlen + 1)) {
            return -1;
        }
    }

    if (NULL == osp->buf) {
        return -1;
    }

    /*
    if (osp->size_data < newlen) {
        memset ((void *) (((opaque_t *) (osp->buf)) + osp->size_data), 0, (size_t) (newlen - osp->size_data));
    }
    OSTR_ZEROTAIL (osp->buf, newlen);
    */
    osp->size_data = newlen;
    osp->buf[newlen] = 0;

    OSTR_CHK_INTEGRITY (osp);
    return newlen;
}

/**
 * @brief 删除 ostr_t 指定位置的数据
 *
 * @param osp : ostr_t 指针
 * @param offset : 要删除的数据起始位置
 * @param len : 要删除的数据长度
 *
 * @return 成功返回ostr_t 删除的数据长度
 *
 * 删除 ostr_t 指定位置的数据
 */
ssize_t
ostr_delete (ostr_t * osp, size_t offset, size_t len)
{
    size_t other_len;

    TRACE_FUNC (ostr_delete);
    ostr_assert (NULL != osp);
    CHK_OSTR_RETURN (osp, -1);
    OSTR_CHK_INTEGRITY (osp);

    if (osp->size_data <= offset)
        return 0;
    other_len = osp->size_data - offset;
    if (other_len < len)
        len = other_len;

    if (offset + len < osp->size_data)
    {
        ostrmem_memmove (((opaque_t *) (osp->buf)) + offset, osp->buf + offset + len, osp->size_data - offset - len);
        osp->count_copy++;
    }
    osp->size_data -= len;
    /*osp->buf[osp->size_data] = 0; */
    OSTR_ZEROTAIL (osp->buf, osp->size_data);

    OSTR_CHK_INTEGRITY (osp);
    return len;
}

/*int ostr_getchar (ostr_t * osp, size_t offset, char *ret_char); */
#if 0
int
ostr_getchar_by_ret (ostr_t * osp, size_t offset)
{
    char ret_char;
    TRACE_FUNC (ostr_getchar_by_ret);
    ostr_assert (NULL != osp);
    OSTR_CHK_INTEGRITY (osp);
    if (0 == ostr_copy_to_bulk (osp, offset, 1, &ret_char))
        return ret_char;
    return '\0';
}
#endif

int
ostr_putchar (ostr_t * osp, size_t offset, unsigned char ch)
{
    TRACE_FUNC (ostr_putchar);
    ostr_assert (NULL != osp);
    OSTR_CHK_INTEGRITY (osp);
    return ostr_copy_from_bulk (osp, offset, 1, (opaque_t *)(&ch));
}

int
ostr_memset (ostr_t * osp, size_t offset, size_t len, opaque_t value)
{
    TRACE_FUNC (ostr_memset);
    /* CHK_OSTR_RETURN (osp, -1); */
    ostr_assert (NULL != osp);
    if (len < 1)
        return 0;
    ostr_assert (len > 0);
    ostr_assert (NULL != osp->buf);    /* CHK_OSTR_RETURN (osp->buf, 0); */
    OSTR_CHK_INTEGRITY (osp);

    if (offset >= osp->size_buf)
        return 0;

    if (offset + len > osp->size_buf)
        len = osp->size_buf - offset;

    memset ((void *) (((opaque_t *) (osp->buf)) + offset), (int) value, len);
    /*osp->buf[osp->size_data] = 0; */
    OSTR_ZEROTAIL (osp->buf, osp->size_data);

    OSTR_CHK_INTEGRITY (osp);
    return len;
}

ssize_t
ostr_append_values (ostr_t * op, size_t size, opaque_t value)
{
    size_t offset;
    TRACE_FUNC (ostr_append_values);
    offset = op->size_data;
    if (0 <= ostr_truncate (op, offset + size))
    {
        if (0 <= ostr_memset (op, offset, size, value))
            return size;
    }
    return -1;
}

#define MAX(a,b) (((a) > (b))?(a):(b))
#define MIN(a,b) (((a) < (b))?(a):(b))

/* (osp == bulk)?0:(osp < bulk)?-1:1 */
int
ostr_memcmp_bulk (ostr_t * osp, size_t offset, size_t len, opaque_t * bulk)
{
    size_t len_osp;
    size_t len_real;
    int ret;

    TRACE_FUNC (ostr_memcmp_bulk);
    ostr_assert (NULL != osp);
    ostr_assert (NULL != bulk);
    CHK_OSTR_RETURN (bulk, -2);
    CHK_OSTR_RETURN (osp, -2);
    OSTR_CHK_INTEGRITY (osp);

    if (offset >= osp->size_data)
        return -2;

    len_osp = osp->size_data - offset;
    len_real = MIN (len_osp, len);

    ret = memcmp (((opaque_t *) (osp->buf)) + offset, bulk, len_real);

    OSTR_CHK_INTEGRITY (osp);
    if (0 == ret)
    {
        if (len_real < len)
            return -1;
    }

    return ret;
}

int
ostr_memcmp_peer (ostr_t * osp, size_t offset, size_t len, ostr_t * peer, size_t peer_offset, size_t peer_len)
{
    size_t len_peer;
    size_t len_real;
    int ret;

    TRACE_FUNC (ostr_memcmp_peer);
    ostr_assert (NULL != osp);  /* CHK_OSTR_RETURN (osp, -3); */
    ostr_assert (NULL != peer); /* CHK_OSTR_RETURN (peer, -3); */
    OSTR_CHK_INTEGRITY (osp);

    if (peer_offset >= peer->size_data)
        return -3;

    len_peer = peer->size_data - peer_offset;
    if (peer_len > len_peer) {
        peer_len = len_peer;
    }
    len_real = len;
    if (offset + len > osp->size_data) {
        len_real = osp->size_data - offset;
    }
    len_real = MIN (peer_len, len_real);

    ret = ostr_memcmp_bulk (osp, offset, len_real, peer->buf + peer_offset);

    OSTR_CHK_INTEGRITY (osp);
    if ((0 == ret) && ((len_real != len) || (len_real != peer_len)))
    {
        if (peer_len < len)
            return 1;
        else if (peer_len > len)
            return -1;
    }

    return ret;
}

/* 在缓冲中搜索二进制串，效率有待提高 */
ssize_t
ostr_search_bulk (ostr_t * osp, size_t offset_base, const uint8_t *needle, size_t sz_neddle)
{
    uint8_t *p;
    TRACE_FUNC (ostr_search_bulk);
    ostr_assert (NULL != osp);    /* CHK_OSTR_RETURN (osp, -3); */
    ostr_assert (NULL != needle); /* CHK_OSTR_RETURN (peer, -3); */
    OSTR_CHK_INTEGRITY (osp);

    /*assert (offset_base < osp->size_data); */
    if (offset_base >= osp->size_data) {
        return -1;
    }

    if (sz_neddle == 0) {
        return 0;
    }

    if (sz_neddle == 1) {
        p = (uint8_t *)memchr (osp->buf + offset_base, *needle, osp->size_data - offset_base);
        OSTR_CHK_INTEGRITY (osp);
        if (p == NULL) {
            return -1;
        }
        assert (p >= osp->buf);
        return (p - osp->buf);
    }

    p = (uint8_t *)memchr (osp->buf + offset_base, *needle, osp->size_data - offset_base);
    for (; NULL != p && osp->size_data - (p - osp->buf) >= sz_neddle; ) {
        if (memcmp(p, needle, sz_neddle) == 0) {
            OSTR_CHK_INTEGRITY (osp);
            return (p - osp->buf);
        }
        p = (uint8_t *)memchr (p + 1, *needle, osp->size_data - (p - osp->buf));
    }

    OSTR_CHK_INTEGRITY (osp);
    return -1;
}

void * memcasechr (void *buf, int ch, size_t szbuf);

void *
memcasechr (void *buf, int ch, size_t szbuf)
{
    uint8_t *p;
    ch = toupper (ch);
    p = (uint8_t *)buf;
    for (; szbuf > 0;) {
        if (ch == toupper (*p)) {
            return p;
        }
        p ++;
        szbuf --;
    }
    return NULL;
}

int
memcasecmp (const char * buf1, const char * buf2, size_t szbuf)
{
    uint8_t val1;
    uint8_t val2;
    for (; szbuf > 0; ) {
        val1 = toupper (*buf1);
        val2 = toupper (*buf2);
        if (val1 > val2) {
            return 1;
        }
        if (val1 < val2) {
            return -1;
        }
        szbuf --;
        buf1 ++;
        buf2 ++;
    }
    return 0;
}

int
ostr_memcasecmp_bulk (ostr_t * osp, size_t offset, size_t len, opaque_t * bulk)
{
    size_t len_osp;
    size_t len_real;
    int ret;

    TRACE_FUNC (ostr_memcmp_bulk);
    ostr_assert (NULL != osp);
    ostr_assert (NULL != bulk);
    CHK_OSTR_RETURN (bulk, -2);
    CHK_OSTR_RETURN (osp, -2);
    OSTR_CHK_INTEGRITY (osp);

    if (offset >= osp->size_data)
        return -2;

    len_osp = osp->size_data - offset;
    len_real = MIN (len_osp, len);
    assert (len_real <= len);

    ret = memcasecmp (((char *) (osp->buf)) + offset, (char *)bulk, len_real);

    OSTR_CHK_INTEGRITY (osp);
    if (0 == ret)
    {
        if (len_real < len)
            return -1;
    }

    return ret;
}

ssize_t
ostr_casecstr_bulk (ostr_t * osp, size_t offset_base, const uint8_t *needle, size_t sz_neddle)
{
    uint8_t *p;
    TRACE_FUNC (ostr_search_bulk);
    ostr_assert (NULL != osp);    /* CHK_OSTR_RETURN (osp, -3); */
    ostr_assert (NULL != needle); /* CHK_OSTR_RETURN (peer, -3); */
    OSTR_CHK_INTEGRITY (osp);

    if (sz_neddle == 0) {
        return 0;
    }

    if (sz_neddle == 1) {
        p = (uint8_t *)memcasechr (osp->buf + offset_base, *needle, osp->size_data - offset_base);
        return (p - osp->buf);
    }

    p = (uint8_t *)memcasechr (osp->buf + offset_base, *needle, osp->size_data - offset_base);
    for (; NULL != p && osp->size_data - (p - osp->buf) >= sz_neddle; ) {
        if (memcasecmp((char *)p, (char *)needle, sz_neddle) == 0) {
            return (p - osp->buf);
        }
        p = (uint8_t *)memcasechr (p + 1, *needle, osp->size_data - (p - osp->buf));
    }

    return -1;
}

/* read to the start of */
ssize_t
ostr_read_callback (ostr_t * osp, size_t offset_osp, size_t len, void *fd, ostr_cb_fsys_read_t cb_read)
{
    ssize_t size_ret = 0;
    opaque_t *data;
    ssize_t ret;

    TRACE_FUNC (ostr_read_fd);
    /* CHK_OSTR_RETURN (osp, -1); */
    ostr_assert (NULL != osp);
    OSTR_CHK_INTEGRITY (osp);

    if (len < 1) {
        DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_WARNING, "request 0 size");
        return 0;
    }
    if (offset_osp > osp->size_buf) {
        DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "offset exceed range");
        return -2;
    }
    if (offset_osp + len > osp->size_buf)
    {
        ostr_require_space (osp, offset_osp + len);
    }

    data = osp->buf + offset_osp;
    if (len > osp->size_buf)
    {
        len = osp->size_buf;
    }
    while (len > 0)
    {
        ret = cb_read (fd, data, len);
        if (ret <= 0)
        {
            if (errno != EINTR)
            {
                if (size_ret < 1)       /* size_ret == 0 */
                    size_ret = -1;
                DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_WARNING
                    , "no other data read, errno=%d, ret=%"PRIuSZ""
                    , errno, ret
                    );
                break;
            }
        }
        else
        {
            /* ret may be less than len */
            len -= ret;
            data += ret;
            size_ret += ret;
        }
    }
    if (size_ret > 0)
    {
        osp->size_data += size_ret;
    }

    if (osp->size_data < osp->size_buf)
    {
        /* osp->buf [osp->size_data] = 0; */
        OSTR_ZEROTAIL (osp->buf, osp->size_data);
    }

    OSTR_CHK_INTEGRITY (osp);
    return size_ret;
}

ssize_t
ostr_write_callback (ostr_t * osp, size_t offset, size_t len, void *fd, ostr_cb_fsys_write_t cb_write)
{
    ssize_t size_ret = 0;
    opaque_t *data;
    ssize_t ret;

    TRACE_FUNC (ostr_write_fd);
    /* CHK_OSTR_RETURN (osp, -1); */
    ostr_assert (NULL != osp);
    OSTR_CHK_INTEGRITY (osp);

    if (len < 1)
        return len;

    if (offset > osp->size_data)
        return -2;
    if (offset + len > osp->size_data)
        len = osp->size_data - offset;
    if (len < 1)
        return len;

    data = osp->buf + offset;
    while (len > 0)
    {
        ret = cb_write (fd, data, len);
        if (ret <= 0)
        {
            if (errno != EINTR)
            {
                if (size_ret < 1)       /* size_ret == 0 */
                    size_ret = -1;
                break;
            }
        }
        else
        {
            /* ret may be less than len */
            len -= ret;
            data += ret;
            size_ret += ret;
        }
    }
    OSTR_CHK_INTEGRITY (osp);
    return size_ret;
}

static ssize_t
ostr_cb_fsys_read_fd (void *fd, void * buffer, size_t size)
{
    ssize_t ret;
    assert (sizeof (void *) >= sizeof (int));
    ret = read ((intptr_t)fd, buffer, size);
    return ret;
}

static ssize_t
ostr_cb_fsys_write_fd (void *fd, void * buffer, size_t size)
{
    ssize_t ret;
    assert (sizeof (void *) >= sizeof (int));
    ret = write ((intptr_t)fd, buffer, size);
    return ret;
}

/* read to the start of */
ssize_t
ostr_read_fd (ostr_t * osp, size_t offset_osp, size_t len, int fd)
{
    assert (sizeof (void *) >= sizeof (int));
    return ostr_read_callback (osp, offset_osp, len, (void *) (intptr_t)fd, ostr_cb_fsys_read_fd);
}

ssize_t
ostr_write_fd (ostr_t * osp, size_t offset_osp, size_t len, int fd)
{
    assert (sizeof (void *) >= sizeof (int));
    return ostr_write_callback (osp, offset_osp, len, (void *) (intptr_t)fd, ostr_cb_fsys_write_fd);
}

#if 1
static ssize_t
ostr_cb_fsys_read_stream (void *fp, void * buffer, size_t size)
{
    size_t ret;
    assert (sizeof (void *) >= sizeof (FILE *));
    ret = fread (buffer, 1, size, (FILE *)fp);
    if (ret == 0) {
        ret = -1;
    }
    return ret;
}

static ssize_t
ostr_cb_fsys_write_stream (void *fp, void * buffer, size_t size)
{
    ssize_t ret;
    assert (sizeof (void *) >= sizeof (FILE *));
    ret = fwrite (buffer, 1, size, (FILE *)fp);
    if (ret == 0) {
        ret = -1;
    }
    return ret;
}

ssize_t
ostr_read_stream (ostr_t * osp, size_t offset_osp, size_t len, FILE * fp)
{
    assert (sizeof (void *) >= sizeof (FILE *));
    return ostr_read_callback (osp, offset_osp, len, (void *)fp, ostr_cb_fsys_read_stream);
}

ssize_t
ostr_write_stream (ostr_t * osp, size_t offset_osp, size_t len, FILE *fp)
{
    assert (sizeof (void *) >= sizeof (FILE *));
    return ostr_write_callback (osp, offset_osp, len, (void *)fp, ostr_cb_fsys_write_stream);
}

#else
ssize_t
ostr_read_stream (ostr_t * osp, size_t offset_osp, size_t len, FILE * fp)
{
    ssize_t size_ret = 0;
    opaque_t *data;
    ssize_t ret;

    TRACE_FUNC (ostr_read_fd);
    /* CHK_OSTR_RETURN (osp, -1); */
    ostr_assert (NULL != osp);
    OSTR_CHK_INTEGRITY (osp);
    if (NULL == fp)
        return -1;

    if (len < 1)
    {
        return 0;
    }
    if (offset_osp > osp->size_buf)
    {
        return -2;
    }
    if (offset_osp + len > osp->size_buf)
    {
        ostr_require_space (osp, offset_osp + len);
    }

    data = osp->buf + offset_osp;
    if (len > osp->size_buf)
    {
        len = osp->size_buf;
    }
    while (len > 0)
    {
        ret = fread (data, 1, len, fp);
        if (ret < 1)
        {
            if (errno != EINTR)
            {
                if (size_ret < 1)       /* size_ret == 0 */
                    size_ret = -1;
                break;
            }
        }
        else
        {
            /* ret may be less than len */
            len -= ret;
            data += ret;
            size_ret += ret;
        }
    }
    if (size_ret > 0)
    {
        osp->size_data += size_ret;
    }

    if (osp->size_data < osp->size_buf)
    {
        /* osp->buf [osp->size_data] = 0; */
        OSTR_ZEROTAIL (osp->buf, osp->size_data);
    }

    OSTR_CHK_INTEGRITY (osp);
    return size_ret;
}

ssize_t
ostr_write_stream (ostr_t * osp, size_t offset_osp, size_t len, FILE *fp)
{
    ssize_t size_ret = 0;
    opaque_t *data;
    ssize_t ret;

    TRACE_FUNC (ostr_write_fd);
    /* CHK_OSTR_RETURN (osp, -1); */
    ostr_assert (NULL != osp);
    OSTR_CHK_INTEGRITY (osp);
    if (NULL == fp) {
        return -1;
    }

    if (len < 1) {
        return len;
    }

    if (offset_osp > osp->size_data)
        return -2;
    if (offset_osp + len > osp->size_data)
        len = osp->size_data - offset_osp;
    if (len < 1)
        return len;

    data = osp->buf + offset_osp;
    while (len > 0)
    {
        ret = fwrite (data, 1, len, fp);
        if (ret < 1)
        {
            if (errno != EINTR)
            {
                if (size_ret < 1)       /* size_ret == 0 */
                    size_ret = -1;
                break;
            }
        }
        else
        {
            /* ret may be less than len */
            len -= ret;
            data += ret;
            size_ret += ret;
        }
    }
    OSTR_CHK_INTEGRITY (osp);
    return size_ret;
}
#endif

int ostr_get_bits (ostr_t * osp, size_t bit_offset, size_t numbits, opaque_t * bulk);
int ostr_set_bits (ostr_t * osp, size_t bit_offset, size_t numbits, opaque_t * bulk);

static ssize_t
ostr_format_valist (ostr_t * osp, size_t offset, const char *fmt, va_list args)
{
    opaque_t *p;
    ssize_t n;
    ssize_t size;

    TRACE_FUNC (ostr_format_valist);
    ostr_assert (NULL != osp);
    ostr_assert (NULL != fmt);
    size = strlen (fmt);

    if (0 > ostr_require_space (osp, offset + size))
        return -1;

    for (;;)
    {
        p = osp->buf + offset;
        size = ostr_left_space (osp);

        n = vsnprintf ((char *)p, size, fmt, args);

        if (n > -1 && n < size)
        {
            osp->size_data = n + offset;
            return n;
        }
#if 0
        /* Else try again with more space. */
        if (n > -1)             /* glibc 2.1 */
            size = n + 1;       /* precisely what is needed */
        else                    /* glibc 2.0 */
            size *= 2;          /* twice the old size */
#endif
        size++;

        if (0 > ostr_require_more_space (osp, size))
            return -1;
    }
    return -1;
}

ssize_t
ostr_format (ostr_t * osp, size_t offset, const char *fmt, ...)
{
    va_list args;
    ssize_t ret;

    ostr_assert (NULL != osp);
    CHK_OSTR_RETURN (osp, -1);
    OSTR_CHK_INTEGRITY (osp);

    va_start (args, fmt);
    ret = ostr_format_valist (osp, offset, fmt, args);
    va_end (args);

    OSTR_CHK_INTEGRITY (osp);
    return ret;
}

ssize_t
ostr_format_append (ostr_t * osp, const char *fmt, ...)
{
    va_list args;
    ssize_t ret;

    ostr_assert (NULL != osp);
    CHK_OSTR_RETURN (osp, -1);
    OSTR_CHK_INTEGRITY (osp);

    va_start (args, fmt);
    ret = ostr_format_valist (osp, osp->size_data, fmt, args);
    va_end (args);

    OSTR_CHK_INTEGRITY (osp);
    return ret;
}

ostr_t *
ostr_format_new (const char *fmt, ...)
{
    ostr_t *osp;
    va_list args;
    ssize_t ret;

    osp = ostr_create (0);
    if (NULL == osp)
        return NULL;

    va_start (args, fmt);
    ret = ostr_format_valist (osp, osp->size_data, fmt, args);
    va_end (args);

    OSTR_CHK_INTEGRITY (osp);
    if (ret > 0)
        return osp;
    ostr_destroy (osp);
    return NULL;
}

static int
bhd_cb_writer_null (void *fd, opaque_t * fragment, size_t size)
{
    return size;
}

size_t
bulk_hex_dump (void *fd, opaque_t * fragment, size_t size, bhd_cb_writer_t writer, int use_c_style)
{
    bhd_cb_writer_t pwriter = bhd_cb_writer_null;
    size_t size_ret = 0;
    int ret = 0;
    size_t line_num = 0;
    size_t i = 0;
    uint8_t buffer[20];
    /* printf("the buffer data(size=%d=0x%08x):\n", size, size); */
    if (NULL != writer) {
        pwriter = writer;
    }
    i = line_num << 4;
    while (i < size)
    {
        if (! use_c_style)
        {
            ret = sprintf ((char *)buffer, "%08xh: ", (unsigned int)i);
            assert (ret < (int)sizeof (buffer));
            if (ret > 0)
            {
                ret = pwriter (fd, buffer, ret);
                if (ret > 0)
                    size_ret += ret;
            }
        }
        while ((i < ((line_num + 1) << 4)))
        {
            if (i == (line_num << 4) + 8)
            {
                if (use_c_style) {
                    ret = pwriter (fd, (uint8_t *)" ", strlen (" "));
                } else {
                    ret = pwriter (fd, (uint8_t *)"- ", strlen ("- "));
                }
                if (ret > 0)
                    size_ret += ret;
            }

            if (i < size)
            {
                if (use_c_style) {
                    ret = sprintf ((char *)buffer, "0x%02X, ", fragment[i]);
                    assert (ret < (int)sizeof (buffer));
                } else {
                    ret = sprintf ((char *)buffer, "%02X ", fragment[i]);
                    assert (ret < (int)sizeof (buffer));
                }
                if (ret > 0)
                {
                    ret = pwriter (fd, buffer, ret);
                    if (ret > 0)
                        size_ret += ret;
                }
            }
            else
            {
                ret = pwriter (fd, (uint8_t *)"   ", strlen ("   "));
                if (ret > 0)
                    size_ret += ret;
            }

            i++;
        }
        if (! use_c_style) {
            ret = pwriter (fd, (uint8_t *)"; ", strlen ("; "));
            if (ret > 0)
                size_ret += ret;
            i = line_num << 4;
            while ((i < ((line_num + 1) << 4)) && (i < size))
            {
                char ch;
                if (i == (line_num << 4) + 8)
                {
                    ret = pwriter (fd, (uint8_t *)" ", strlen (" "));
                    if (ret > 0)
                        size_ret += ret;
                }
                ch = fragment[i];
                if (!isprint (ch))
                    ch = '.';
                ret = sprintf ((char *)buffer, "%c", ch);
                assert (ret < (int)sizeof (buffer));
                ret = pwriter (fd, buffer, ret);
                if (ret > 0)
                    size_ret += ret;
                i++;
            }
        }
        ret = pwriter (fd, (uint8_t *)"\n", strlen ("\n"));
        if (ret > 0)
            size_ret += ret;
        line_num++;
        i = line_num << 4;
    }
    return size_ret;
}

int
ostr_cb_writer_fdwrite (void *fd, opaque_t * fragment, size_t size)
{
    return write ((intptr_t)fd, fragment, size);
}

#if defined(USE_CSTYLE)
#define hex_dump_to_fd_forostr(fd, fragment, size) bulk_hex_dump ((void *)(intptr_t)(fd), fragment, size, ostr_cb_writer_fdwrite, 1)
#else
#define hex_dump_to_fd_forostr(fd, fragment, size) bulk_hex_dump ((void *)(intptr_t)(fd), fragment, size, ostr_cb_writer_fdwrite, 0)
#endif

/* 打印到内存缓冲区中 */
typedef struct _mem_d2b_writer_info_t {
  char *buffer;
  size_t cursize;
  size_t maxsize;
} mem_d2b_writer_info_t;

static int
_mem_d2b_writer (void * fd, opaque_t * fragment, size_t size)
{
    mem_d2b_writer_info_t *pbufinfo = (mem_d2b_writer_info_t *)fd;
    assert (NULL != pbufinfo);
    if (pbufinfo->cursize >= pbufinfo->maxsize)
    {
        return -1;
    }
    if (pbufinfo->cursize + size >= pbufinfo->maxsize)
    {
        size = pbufinfo->maxsize - pbufinfo->cursize;
    }
    ostrmem_memmove (pbufinfo->buffer + pbufinfo->cursize, fragment, size);
    pbufinfo->cursize += size;
    return size;
}

/**
 * @brief 将内容通过十六进制格式输出到 C内存
 *
 * @param osp : 内容所在缓冲
 * @param max_size : 输出缓冲大小
 * @param bulk : 输出缓冲
 *
 * @return 成功返回输出信息字节个数
 *
 * 将内容通过十六进制格式输出到 C内存
 */
ssize_t
ostr_dump_to_bulk (ostr_t * osp, size_t max_size, opaque_t * bulk)
{
    mem_d2b_writer_info_t meminfo;
    /*ostr_assert(NULL != osp); */
    CHK_OSTR_RETURN (osp, -1);
    meminfo.buffer = (char *)bulk;
    meminfo.cursize = 0;
    meminfo.maxsize = max_size;
    return bulk_hex_dump ((void *)(&meminfo), osp->buf, osp->size_data, _mem_d2b_writer, 0);
}

/* 打印到ostr_t的后部 */
static int
_ostr_d2b_writer (void * fd, opaque_t * fragment, size_t size)
{
    if (0 == ostr_append_from_bulk ((ostr_t *)fd, size, fragment))
    {
        return size;
    }
    return -1;
}

/**
 * @brief 将内容通过十六进制格式输出到另一个 ostr_t
 *
 * @param osp : 内容所在缓冲
 * @param osp_peer : 输出缓冲
 *
 * @return 成功返回输出信息字节个数
 *
 * 将内容通过十六进制格式输出到另一个 ostr_t
 */
ssize_t
ostr_dump_to_peer (ostr_t * osp, ostr_t * osp_peer)
{
    /*ostr_assert(NULL != osp); */
    CHK_OSTR_RETURN (osp, -1);
    return bulk_hex_dump (osp_peer, osp->buf, osp->size_data, _ostr_d2b_writer, 0);
}

/**
 * @brief 将内容通过十六进制格式输出到文件描述符指定的文件中
 *
 * @param osp : 内容所在缓冲
 * @param max_size : 输出数据最大长度
 * @param fd : 文件描述符
 *
 * @return 成功返回输出信息字节个数
 *
 * 将内容通过十六进制格式输出到文件描述符指定的文件中
 */
ssize_t
ostr_dump_to_fd (ostr_t * osp, size_t max_size, int fd)
{
    char buffer[200];
    ssize_t ret;
    ssize_t size_ret = 0;
    const char *str_null = "(NULL)ostr_t";

    if (NULL == osp)
        return write (fd, str_null, strlen (str_null));

    if (fd < 0) {
        return -1;
    }

    ret = sprintf (buffer,
                   "\tsize_buf = %"PRIuSZ"\n"
                   "\tsize_data = %"PRIuSZ"\n"
                   "\tsize_request_more = %"PRIuSZ"\n"
                   "\tcount_request = %"PRIuSZ"\n"
                   "\tcount_copy = %"PRIuSZ"\n",
                   osp->size_buf, osp->size_data, osp->size_request_more, osp->count_request, osp->count_copy);
    assert (ret < (ssize_t)sizeof (buffer));
    if (ret <= 0) {
        return -1;
    }

    ret = write (fd, buffer, ret);
    if (ret > 0) {
        size_ret += ret;
    }
    ret = sprintf (buffer, "\tdata: (size=%"PRIuSZ")", osp->size_data);
    assert (ret < (ssize_t)sizeof (buffer));
    ret = write (fd, buffer, ret);
    if (ret > 0) {
        size_ret += ret;
    }
    if (max_size > osp->size_data) {
        ret = write (fd, "\n", 1);
        if (ret > 0) {
            size_ret += ret;
        }
        assert (osp->size_data >= 0);
        max_size = osp->size_data;
    } else {
        ret = sprintf (buffer, "\t *** DUMP size=%"PRIuSZ" ***\n", max_size);
        assert (ret < (ssize_t)sizeof (buffer));
        ret = write (fd, buffer, ret);
        if (ret > 0) {
            size_ret += ret;
        }
    }
    if (osp->buf)
    {
        ostr_assert (NULL != osp->buf);
        ret = hex_dump_to_fd_forostr (fd, osp->buf, max_size);
        if (ret > 0)
            size_ret += ret;
    }

    return size_ret;
}

/**
 * @brief 将内容通过十六进制格式通过回调函数输出
 *
 * @param osp : 内容所在缓冲
 * @param msg : 输出内容前的信息
 * @param startpos : 内容起始位置
 * @param max_size : 输出数据最大长度
 * @param writer : 回调函数
 * @param fd : 传回给回调函数的参数
 *
 * @return 成功返回输出信息字节个数
 *
 * 将内容通过十六进制格式通过回调函数输出
 */
ssize_t
ostr_dump_seg_to_callback (ostr_t * osp, const char *msg, size_t startpos, size_t max_size, bhd_cb_writer_t writer, void *fd)
{
    char buffer[200];
    ssize_t size_ret = 0;
    int ret;
    if (NULL == osp->buf) {
        max_size = 0;
    }
    if (startpos >= osp->size_data) {
        max_size = 0;
    } else {
        if (startpos + max_size >= osp->size_data) {
            max_size = osp->size_data - startpos;
        }
    }
    writer (fd, (opaque_t *)msg, strlen (msg));
    if (max_size < 1) {
        ret = writer (fd, (opaque_t *)": 0 byte", strlen (": 0 byte"));
        if (ret > 0) {
            size_ret += ret;
        }
    }
    ret = writer (fd, (opaque_t *)"\n", 1);
    if (ret > 0) {
        size_ret += ret;
    }

    ret = sprintf (buffer,
                   "\tsize_buf = %"PRIuSZ"\n"
                   "\tsize_data = %"PRIuSZ"\n"
                   "\tsize_request_more = %"PRIuSZ"\n"
                   "\tcount_request = %"PRIuSZ"\n"
                   "\tcount_copy = %"PRIuSZ"\n"
                   "\t--->> startpos = %"PRIuSZ"\n"
                   , osp->size_buf, osp->size_data
                   , osp->size_request_more, osp->count_request
                   , osp->count_copy
                   , startpos
                   );
    assert (ret < (int)sizeof (buffer));
    if (ret <= 0) {
        return -1;
    }

    ret = writer (fd, (opaque_t *)buffer, ret);
    if (ret > 0) {
        size_ret += ret;
    }

    if (max_size < 1) {
        return 0;
    }
    ret = bulk_hex_dump (fd, osp->buf + startpos, max_size, writer, 0);
    if (ret > 0) {
        size_ret += ret;
    }

    return size_ret;
}

/**
 * @brief 将内容通过十六进制格式输出到文件描述符指定的文件中
 *
 * @param osp : 内容所在缓冲
 * @param msg : 输出内容前的信息
 * @param startpos : 内容起始位置
 * @param max_size : 输出数据最大长度
 * @param fd : 文件描述符
 *
 * @return 成功返回输出信息字节个数
 *
 * 将内容通过十六进制格式输出到文件描述符指定的文件中
 */
ssize_t
ostr_dump_seg_to_fd (ostr_t * osp, const char *msg, size_t startpos, size_t max_size, int fd)
{
    int ret = 0;
    ssize_t size_ret = 0;
    if (NULL == osp->buf) {
        max_size = 0;
    }
    if (startpos >= osp->size_data) {
        max_size = 0;
    } else {
        if (startpos + max_size >= osp->size_data) {
            max_size = osp->size_data - startpos;
        }
    }
    ret = write (fd, msg, strlen (msg));
    if (ret < 0) {
        fprintf (stderr, "Error in write fd=%d\n", fd);
        return -1;
    }
    size_ret += ret;
    if (max_size < 1) {
        ret = write (fd, ": 0 byte", strlen (": 0 byte"));
        if (ret > 0) {
            size_ret += ret;
        }
    }
    ret = write (fd, "\n", 1);
    if (ret < 0) {
        fprintf (stderr, "Error in write fd=%d\n", fd);
        return -1;
    }
    size_ret += ret;
    if (max_size < 1) {
        return size_ret;
    }
    ostr_assert (NULL != osp->buf);
    ret = hex_dump_to_fd_forostr (fd, osp->buf + startpos, max_size);
    if (ret > 0) {
        size_ret += ret;
    }
    return size_ret;
}

#if 0
ssize_t
ostr_pack_with_length (ostr_t * osp, size_t offset, size_t len, int num_bytes_len, ostr_t * peer, size_t peer_offset)
{
#define GET_BITS_RETURN(bits, len, ret_err) \
do { \
long max = 0x1; \
for(bits = 0; bits < 32; bits ++) \
{ \
  if (max > (long)(len)) \
    break; \
  max <<= 8; \
} \
if(bits >= (int)sizeof(long)) \
  return ret_err; \
} while(0) \

    int bits;
    ssize_t ret;

    TRACE_FUNC (ostr_pack_with_length);
    ostr_assert (NULL != osp);
    CHK_OSTR_RETURN (osp, -1);
    OSTR_CHK_INTEGRITY (osp);

    if (num_bytes_len > 4)
        return -1;

    GET_BITS_RETURN (bits, len, -1);
#undef GET_BITS_RETURN


    if (num_bytes_len < bits)
        len = (0x1 << num_bytes_len) - 1;

    if (offset >= osp->size_data)
        return -1;
    if (offset + len > osp->size_data)
        len = osp->size_data - offset;


    if (0 > ostr_require_space (peer, offset + len + num_bytes_len))
        return -1;

    switch (num_bytes_len)
    {
    case 1:
        {
            opaque_t size;
            size = (opaque_t) (len);
            ret = ostr_copy_from_bulk (peer, peer_offset, sizeof (size), (opaque_t *) (&size));
            peer_offset += sizeof (size);
            break;
        }
    case 2:
        {
            uint16_t size;
            size = htons ((uint16_t) (len));
            ret = ostr_copy_from_bulk (peer, peer_offset, sizeof (size), (opaque_t *) (&size));
            peer_offset += sizeof (size);
            break;
        }
    case 4:
        {
            uint32_t size;
            size = htonl ((uint32_t) (len));
            ret = ostr_copy_from_bulk (peer, peer_offset, sizeof (size), (opaque_t *) (&size));
            peer_offset += sizeof (size);
            break;
        }
    default:
        {
            OSTR_CHK_INTEGRITY (osp);
            return -1;
            break;
        }
    }
    OSTR_CHK_INTEGRITY (osp);
    return (ret + ostr_copy_from_peer (peer, peer_offset, len, osp, offset));
}

ostr_t *
ostr_pack_with_length_new (ostr_t * osp, size_t offset, size_t len, int num_bytes_len)
{
    ostr_t *peer;

    ostr_assert (NULL != osp);
    CHK_OSTR_RETURN (osp, NULL);
    OSTR_CHK_INTEGRITY (osp);
    peer = ostr_create (0);
    if (NULL == peer)
        return NULL;

    if (0 > ostr_pack_with_length (osp, offset, len, num_bytes_len, peer, 0))
    {
        ostr_destroy (peer);
        return NULL;
    }

    OSTR_CHK_INTEGRITY (osp);
    OSTR_CHK_INTEGRITY (peer);
    return peer;
}
#endif // 0

int
ostr_getval_uint32 (ostr_t *osp, size_t pos, uint32_t * val_ret)
{
    if (NULL == val_ret)
    {
        return -1;
    }
    if (pos + 4 > osp->size_data)
    {
        return -1;
    }
#if 0
    *val_ret = *(osp->buf + pos);
#else
    *val_ret = *((uint32_t *)(&(osp->buf[pos])));
#endif
    return 0;
}

#if TEST_OCTSTR

#define TEST_TRACE DS_TRACE

int
test_ostr_create (void)
{
    ostr_t *osp = NULL;
    ostr_t *osp_bulk = NULL;
    char *bulk = "### test string 0001 ###";
    int ret = 0;

    TEST_TRACE (("+++ start test +++\n"));

    osp = ostr_create (0);
    if (NULL == osp)
        ret = -1;
    TEST_TRACE (("ostr_dump_to_stream(osp) = %d:\n", ostr_dump_to_stream (osp, stdout)));

    osp_bulk = ostr_create_from_bulk (bulk, strlen (bulk));
    if (NULL == osp_bulk)
        ret = -2;
    TEST_TRACE (("ostr_dump_to_stream(osp_bulk) = %d:\n", ostr_dump_to_stream (osp_bulk, stdout)));

    if (0 > ostr_destroy (osp))
        ret = -3;

    osp = ostr_create_from_peer (osp_bulk);
    if (NULL == osp)
        ret = -4;
    TEST_TRACE (("ostr_dump_to_stream(osp) = %d:\n", ostr_dump_to_stream (osp, stdout)));

    if (0 > ostr_destroy (osp))
        ret = -5;
    if (0 > ostr_destroy (osp_bulk))
        ret = -6;
    TEST_TRACE (("--- end test ---\n\n"));
    return ret;
}

int
test_ostr_require_space (void)
{
    int ret = 0;
    int i;
    ostr_t *osp = NULL;

    TEST_TRACE (("+++ start test +++\n"));
    osp = ostr_create (0);
    if (NULL == osp)
        ret = -1;

    TEST_TRACE (("ostr_require_space(osp, 2) = %d:\n", ostr_require_space (osp, 2)));

    for (i = 50; i > 0; i--)
        TEST_TRACE (("ostr_require_more_space(osp, 2) = %d:\n", ostr_require_more_space (osp, 2)));

    TEST_TRACE (("ostr_dump_to_stream(osp) = %d:\n", ostr_dump_to_stream (osp, stdout)));

    TEST_TRACE (("--- end test ---\n\n"));
    if (0 > ostr_destroy (osp))
        ret = -5;
    if (NULL == osp)
        return -1;
    return 0;
}

int
test_ostr_copy (void)
{
    int ret = 0;
    ostr_t *osp = NULL;
    ostr_t *osp_bulk = NULL;
    char *bulk = "### test string 0001 ###";
    char bulk_to[200];

    TEST_TRACE (("+++ start test +++\n"));
    osp = ostr_create (0);
    if (NULL == osp)
        return -1;
    osp_bulk = ostr_create_from_bulk (bulk, strlen (bulk));
    if (NULL == osp_bulk)
    {
        ostr_destroy (osp);
        return -1;
    }

    TEST_TRACE (("ostr_copy_from_bulk(osp, 35, 2, bulk) = %d:\n", ostr_copy_from_bulk (osp, 35, 2, bulk)));
    TEST_TRACE (("ostr_dump_to_stream(osp):\n"));
    ostr_dump_to_stream (osp, stdout);
    TEST_TRACE (("\n\n"));

    TEST_TRACE (("ostr_copy_from_bulk(osp_bulk, 35, 2, bulk) = %d:\n", ostr_copy_from_bulk (osp_bulk, 35, 2, bulk)));
    TEST_TRACE (("ostr_dump_to_stream(osp_bulk):\n"));
    ostr_dump_to_stream (osp_bulk, stdout);
    TEST_TRACE (("\n\n"));

    DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_VERBOSE, "osp.length = %d, osp.left_space = %d\n", ostr_length (osp), ostr_left_space (osp));
    TEST_TRACE (("ostr_dump_to_stream(osp, stdout) = %d:\n", ostr_dump_to_stream (osp, stdout)));
    TEST_TRACE (("\n\n"));

    ret = ostr_copy_to_bulk (osp, 1, sizeof (bulk_to), bulk_to);
    TEST_TRACE (("ostr_copy_to_bulk(osp, 1, sizeof(bulk_to), bulk_to) = %d:\n", ret));
    if (ret > 0)
    {
        TEST_TRACE (("%s's fragment at 0x%X:\n", "bulk_to", bulk_to));
        hex_dump_to_fd_forostr (fileno (stdout), bulk_to, ret);
    }
    TEST_TRACE (("\n\n"));

    TEST_TRACE (("ostr_copy_to_peer(osp,osp_bulk) = %d\n", ostr_copy_to_peer (osp, 0, ostr_length (osp), osp_bulk, 0)));
    TEST_TRACE (("ostr_dump_to_stream(osp_bulk) = %d\n", ostr_dump_to_stream (osp_bulk, stdout)));
    TEST_TRACE (("\n\n"));


    ostr_append_from_bulk (osp, strlen (bulk), bulk);
    ostr_append_from_peer (osp_bulk, ostr_length (osp), osp, 0);
    ostr_append_from_peer (osp_bulk, ostr_length (osp), osp, 0);
    ostr_append_from_peer (osp_bulk, ostr_length (osp), osp, 0);
    ostr_prepend_from_bulk (osp, strlen (bulk), bulk);
    ostr_prepend_from_peer (osp, ostr_length (osp_bulk), osp_bulk, 0);
    ostr_prepend_from_peer (osp, ostr_length (osp_bulk), osp_bulk, 0);
    ostr_prepend_from_peer (osp, ostr_length (osp_bulk), osp_bulk, 0);
    ostr_prepend_from_peer (osp, ostr_length (osp_bulk), osp_bulk, 0);
    TEST_TRACE (("ostr_dump_to_stream(osp, stdout) = %d:\n", ostr_dump_to_stream (osp, stdout)));

    TEST_TRACE (("--- end test ---\n\n"));
    if (0 > ostr_destroy (osp))
        ret = -5;
    if (0 > ostr_destroy (osp_bulk))
        ret = -6;
    return 0;
}

#ifndef LITTLEENDIAN
#define LITTLEENDIAN
#endif

int
test_ostr_getval_uint32 (void)
{
    unsigned char ch;
    int ret = 0;
    int i;
    ostr_t *osp;
    uint32_t val32;
    unsigned char buf[][4] = {
#ifdef LITTLEENDIAN
        {0x03, 0x02, 0x01, 0x00},
        {0x04, 0x03, 0x02, 0x01},
#elif defined (BIGENDIAN)
        {0x00, 0x01, 0x02, 0x03},
        {0x01, 0x02, 0x03, 0x04},
#endif
    };

    osp = ostr_create(0);
    for (i = 0; i < 100; i ++)
    {
        ch = (char)i;
        ostr_append_from_bulk (osp, 1, &ch);
    }
    for (i = 0; i < 2; i ++)
    {
        ostr_getval_uint32(osp, i, &val32);
        if (! memcmp (&val32, buf[i], 4))
        {
            TEST_TRACE (("ERROR: test_ostr_getval_uint32()"));
            ret = -1;
        }
    }
    ostr_destroy (osp);
    return ret;
}

int
test_ostr_performance (void)
{
    int ret = 0;
    ostr_t *osp = NULL;
    char *bulk = "### test string 0001 ###";
#define SIZE_BUF 65530
#define NUM_LOOP 50000
    char buffer[SIZE_BUF];
    char buffer_test[SIZE_BUF];
    struct timeval tv_start;
    struct timeval tv_end;
    int size;
    int last_size;

    int size_bulk;
    int p;
    int i;
    int j;
    int k;
    unsigned int timetime;

    TEST_TRACE (("+++ start test +++\n"));
    osp = ostr_create (0);
    if (NULL == osp)
        ret = -1;

    size_bulk = strlen (bulk);
    p = 0;
    for (i = 0; i < SIZE_BUF; i++)
    {
        buffer[i] = bulk[p];
        p++;
        if (size_bulk <= p)
            p = 0;
    }
    /* cat */

    timetime = time (NULL);
    srand (timetime);
    for (k = 5; k > 0; k--)
    {
        /*size = rand() % SIZE_BUF; */
        size = SIZE_BUF / k;
        printf ("size = %d\n", size);

        gettimeofday (&tv_start, 0);
        for (j = NUM_LOOP; j > 0; j--)
        {
            last_size = 0;
            for (i = 0; i < SIZE_BUF / size; i++)
            {
                ostrmem_memmove (buffer_test + last_size, buffer + last_size, size);
                last_size += size;
            }
        }
        gettimeofday (&tv_end, 0);
        printf ("ostrmem_memmove(): loop num: %d, time: %ld\n", NUM_LOOP,
                ((long) (tv_end.tv_sec - tv_start.tv_sec) * 1000000l + (long) (tv_end.tv_usec - tv_start.tv_usec)));

        gettimeofday (&tv_start, 0);
        for (j = NUM_LOOP; j > 0; j--)
            for (i = 0; i < SIZE_BUF / size; i++)
            {
                ostr_truncate (osp, 0);
                last_size = 0;
                for (i = 0; i < SIZE_BUF / size; i++)
                {
                    ostr_append_from_bulk (osp, size, buffer + last_size);
                    last_size += size;
                }
            }
        gettimeofday (&tv_end, 0);
        printf ("ostr_append(): loop num: %d, time: %ld\n", NUM_LOOP,
                ((long) (tv_end.tv_sec - tv_start.tv_sec) * 1000000l + (long) (tv_end.tv_usec - tv_start.tv_usec)));
    }

    if (0 > ostr_destroy (osp))
        ret = -5;
    if (NULL == osp)
        return -1;
    return 0;
}

typedef int (*test_func_t) (void);
typedef struct _test_func_item_t
{
    test_func_t func;
    char *desc;
} test_func_item_t;
#define FUNC_ITEM(func) {func, #func}

test_func_item_t test_funcs[] = {
    FUNC_ITEM (test_ostr_getval_uint32),
    FUNC_ITEM (test_ostr_create),
    FUNC_ITEM (test_ostr_require_space),
    FUNC_ITEM (test_ostr_copy),
    /*FUNC_ITEM (test_ostr_performance), */
};

#define NUM_TEST_FUNC (sizeof(test_funcs) / sizeof(test_func_item_t))

int test_octstring (void)
{
    int ret = 0;
    int i;

    STAT_INIT ();

    for (i = 0; i < NUM_TEST_FUNC; i++)
        if (0 > test_funcs[i].func ())
        {
            ret--;
            printf ("Error in test func: %s\n", test_funcs[i].desc);
        }
#if 0
    char *bulk = "test string 1";

    TEST_TRACE (("ostr_format_append() = %d:\n",
                 ostr_format_append (osp,
                                     "TESTTEESTASETEASASDFASDFASDFASDFSD, sa;dlkfa;sdkfljasdkf, asd;lkfsa;dlkfsad f, \nfunc:%s, file:%s, line:%d",
                                     __FUNCTION__, __FILE__, __LINE__)));
    TEST_TRACE (("ostr_dump_to_stream(osp) = %d\n", ostr_dump_to_stream (osp, stdout)));
    TEST_TRACE (("\n\n"));


    TEST_TRACE (("ostr_destroy(osp_bulk) = %d:\n", ostr_destroy (osp_bulk)));
    TEST_TRACE (("\n\n"));

    TEST_TRACE (("ostr_pack_with_length_new() to osp_bulk\n"));
    osp_bulk = ostr_pack_with_length_new (osp, 0, osp->size_data, 4);
    TEST_TRACE (("ostr_dump_to_stream(osp_bulk) = %d\n", ostr_dump_to_stream (osp_bulk, stdout)));
    TEST_TRACE (("\n\n"));

    ostr_memset (osp, ostr_length (osp), 'X', 0x03);
    TEST_TRACE (("ostr_dump_to_stream(osp) = %d\n", ostr_dump_to_stream (osp, stdout)));

    TEST_TRACE (("ostr_destroy(osp_bulk) = %d:\n", ostr_destroy (osp)));
    TEST_TRACE (("ostr_destroy(osp_bulk) = %d:\n", ostr_destroy (osp_bulk)));
#endif

    /*ostr_stat_dump (); */
    STAT_DESTROY ();
    return 0;
}

#endif /* TEST_OCTSTR */

/*
#ifdef __cplusplus
}
#endif
*/
