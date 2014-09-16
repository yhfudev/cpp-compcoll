/**
 * @file    octstring.h
 * @brief   C 的可伸缩字串结构, 实现类似于C++语言中的 String
 * 包括预留缓存以供将来添加数据的优化
 * 支持二进制数据存储
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2006/02/24
 * @copyright GNU Lesser General Public License v2.0.
 */
#ifndef _DS_OCTET_STRING_H
#define _DS_OCTET_STRING_H

#if ! defined(_MSC_VER)
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>

#else
#if 0
#include <io.h>                 // _write()
#include <stdio.h>
#define write _write
#define read _read
#define vsnprintf _vsnprintf
#define size_t unsigned int
#define ssize_t int
#else
#include "win32.h"
#endif

#endif


#if 0
#include "pfall.h"
#include "sdict_type.h"
#define SL_TRACE(a)

#else
#define DBGMSG(...)

#include <inttypes.h> /* for PRIdPTR PRIiPTR PRIoPTR PRIuPTR PRIxPTR PRIXPTR, SCNdPTR SCNiPTR SCNoPTR SCNuPTR SCNxPTR */
#ifdef __WIN32__                /* or whatever */
#define PRIiSZ "ld"
#define PRIuSZ "Iu"
#else
#define PRIiSZ "zd"
#define PRIuSZ "zu"
#define PRIxSZ "zx"
#define SCNxSZ "zx"
#endif
#define PRIiOFF PRIx64 /*"lld"*/
#define PRIuOFF PRIx64 /*"llu"*/

#endif



#ifdef __cplusplus
extern "C"
{
#endif

#ifndef opaque_t
#define opaque_t uint8_t
#endif

#if DEBUG
#define OSTRMEM_TRACE 1
#endif

/*
#define opaque_t unsigned char
#define uint16_t unsigned short
#define uint32_t unsigned int
*/

/**
 * @brief 可变字符串结构
 */
typedef struct _octet_string_t {
    opaque_t *buf;    /**<数据缓存指针 */
    size_t size_buf;  /**<所申请的整个缓冲的大小 */
    size_t size_data; /**<目前数据的大小 */
    size_t size_request_more; /**<统计申请内存的大小 */
    size_t count_request; /**<统计申请内存次数 */
    size_t count_copy;    /**<统计拷贝次数 */
/*#ifdef _DEBUG */
    char flag_dyn_alloc;
/*#endif // _DEBUG */
} ostr_t;

#define octet_string_t ostr_t

#ifdef _DEBUG
#define OSTR_CHK_INTEGRITY(osp) \
do { \
  if (NULL == osp) \
  { \
    DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "ERROR: osp == NULL"); \
    break; \
  } \
  if (NULL == osp->buf) \
  { \
    DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_WARNING, "WARNING: osp->buf == NULL"); \
    assert ((osp)->buf == NULL); \
    assert ((osp)->size_buf == NULL); \
    assert ((osp)->size_data == 0); \
  } \
  if (osp->size_data > osp->size_buf) { \
    DBGMSG (PFDBG_CATLOG_USR_OSTR, PFDBG_LEVEL_ERROR, "ERROR: osp->size_data(%d) > osp->size_buf(%d)", osp->size_data, osp->size_buf); \
    /*ostr_dump_to_stream(osp, stdout); */ \
    assert (osp->size_data <= osp->size_buf); \
  } \
  assert (osp->size_data < 100 * 1024 * 1024); \
  assert (osp->size_buf < 100 * 1024 * 1024); \
  assert (osp->size_request_more < 100 * 1024 * 1024); \
  assert (osp->count_request < 4 * 1024 * 1024); \
  assert (osp->count_copy < 4 * 1024 * 1024); \
} while(0)
#define OSTR_SELFCHK(osp) OSTR_CHK_INTEGRITY(osp)

#else /* _DEBUG */
#define OSTR_CHK_INTEGRITY(osp)
#define OSTR_SELFCHK(osp)
#endif /* _DEBUG */

/*
 * @NAME
 *   ostr_create, ostr_create_from_bulk, ostr_create_from_bulk, ostr_duplicate
 *   创建一个可伸缩字串
 *
 * @SYNOPSIS
 *   #include "octstring.h"
 *   ostr_t *ostr_create (size_t size);
 *   ostr_t *ostr_create_from_bulk (opaque_t * bulk, size_t size);
 *   ostr_t *ostr_create_from_peer (ostr_t * peer);
 *   ostr_t *ostr_duplicate (ostr_t * peer);
 *
 * @DESCRIPTION
 *   函数 ostr_create 新申请一个 ostr_t 型的内存, 然后初始化它并返回. 初始化时会
 *   预分配size大小的数据空间, 并且数据大小记录置为0, 最后返回该 ostr_t 结构的指针.
 *
 *   函数 ostr_create_from_bulk 先会生成一个 ostr_t 结构, 然后复制bulk所指的size
 *   大小的数据到内部数据存储区中, 最后返回该 ostr_t 结构的指针.
 *
 *   函数 ostr_create_from_peer 和 ostr_duplicate 先会生成一个 ostr_t 结构, 然后
 *   复制peer中的数据到内部数据存储区中, 最后返回该 ostr_t 结构的指针.
 *
 * @RETURN VALUE
 *   如果成功, 返回一个创建好的 ostr_t 结构指针, 否则返回 NULL.
 *
 * @ERRORS
 *   None.
 *
 * @EXAMPLES
 *   None.
 *
 * @APPLICATION USAGE
 *   None.
 *
 * @FUTURE DIRECTIONS
 *   None.
 *
 * @SEE ALSO
 *   ostr_copy_from_bulk(), ostr_append_from_bulk(), ostr_truncate(),
 *   ostr_delete()
 *
 */
#if OSTRMEM_TRACE
ostr_t *ostr_create_dbg (size_t size, const char *filename, int line);
#define ostr_create(size) ostr_create_dbg (size, __FILE__, __LINE__);
#else
ostr_t *ostr_create (size_t size);
#define ostr_create_dbg(size, filename, line, f, l) ostr_create(size, filename, line)
#endif

ostr_t *ostr_create_from_bulk (opaque_t * bulk, size_t size);
/*ostr_t *ostr_create_from_peer (ostr_t * peer); */
#define ostr_create_from_peer(peer) ((NULL == peer)?(NULL):ostr_create_from_bulk((peer)->buf, ostr_length(peer)))
/*ostr_t * ostr_duplicate(ostr_t *osp); */
#define ostr_duplicate(peer) ostr_create_from_peer(peer)

/*
 * @NAME
 *   ostr_destroy
 *   人道毁灭使用 ostr_create_XXX() 生成的一个可伸缩字串
 *
 * @SYNOPSIS
 *   #include "octstring.h"
 *   int ostr_destroy (ostr_t * osp);
 *
 * @DESCRIPTION
 *   函数 ostr_destroy 毁灭一个 ostr_t 型的结构, 释放分配的数据空间. 如果该结构
 *   是 ostr_create(), ostr_create_from_bulk(), ostr_create_from_peer(),
 *   ostr_duplicate() 等函数返回的, 则释放该 ostr_t 结构的内存.
 *
 * @RETURN VALUE
 *   如果成功, 返回零.
 *
 * @ERRORS
 *   None.
 *
 * @EXAMPLES
 *   None.
 *
 * @APPLICATION USAGE
 *   None.
 *
 * @FUTURE DIRECTIONS
 *   None.
 *
 * @SEE ALSO
 *   ostr_init(), ostr_init_from_bulk(), ostr_init_from_peer(),
 *   ostr_create(), ostr_create_from_bulk(), ostr_create_from_peer()
 *   ostr_duplicate(), ostr_delete()
 *
 */
int ostr_destroy (ostr_t * osp);

/*
 * @NAME
 *   ostr_init, ostr_init_from_bulk, ostr_init_from_peer
 *   初始化一个可伸缩字串
 *
 * @SYNOPSIS
 *   #include "octstring.h"
 *   int ostr_init (ostr_t *osp_ref, size_t size);
 *   int ostr_init_from_bulk (ostr_t *osp_ref, opaque_t * bulk, size_t size);
 *   int ostr_init_from_peer (ostr_t *osp_ref, ostr_t * peer);
 *
 * @DESCRIPTION
 *   函数 ostr_init 初始化一个 ostr_t 型的结构, 预分配 size 大小的数据空间,
 *   并且数据大小记录置为0.
 *
 *   函数 ostr_init_from_bulk 初始化一个 ostr_t 型的结构, 然后复制 bulk 所指的
 *   size 大小的数据到内部数据存储区中.
 *
 *   函数 ostr_init_from_peer 初始化一个 ostr_t 型的结构, 然后复制 peer 中的
 *   数据到内部数据存储区中.
 *
 * @RETURN VALUE
 *   如果成功, 返回零.
 *
 * @ERRORS
 *   None.
 *
 * @EXAMPLES
 *   None.
 *
 * @APPLICATION USAGE
 *   None.
 *
 * @FUTURE DIRECTIONS
 *   初始化函数遇到大小为 0 的初始值时，内部不分配内存。
 *
 * @SEE ALSO
 *   ostr_copy_from_bulk(), ostr_append_from_bulk(), ostr_truncate(),
 *   ostr_delete()
 *
 */
#if OSTRMEM_TRACE
int ostr_init_dbg (ostr_t * osp, size_t size, const char * filename, int line);
#define ostr_init(osp, size) ostr_init_dbg(osp, size, __FILE__, __LINE__)
#else
int ostr_init (ostr_t * osp_ref, size_t size);
#define ostr_init_dbg(osp, size, f, l) ostr_init(osp, size)
#endif

int ostr_init_from_bulk (ostr_t * osp_ref, opaque_t * bulk, size_t size);
/*int ostr_init_from_peer (ostr_t *osp_ref, ostr_t * peer); */
#define ostr_init_from_peer(osp_ref, peer) ((NULL == peer)?(-1):ostr_init_from_bulk((osp_ref), (peer)->buf, ostr_length(peer)))

/*
 * @NAME
 *   ostr_clear
 *   清除使用 ostr_init() 初始化的一个可伸缩字串
 *
 * @SYNOPSIS
 *   #include "octstring.h"
 *   int ostr_clear (ostr_t * osp);
 *
 * @DESCRIPTION
 *   函数 ostr_clear 清除一个 ostr_t 型的结构, 释放分配的数据空间. 该结构
 *   必须是 ostr_init(), ostr_init_from_bulk(), ostr_init_from_peer()
 *   等函数返回的.
 *
 * @RETURN VALUE
 *   如果成功, 返回零.
 *
 * @ERRORS
 *   None.
 *
 * @EXAMPLES
 *   None.
 *
 * @APPLICATION USAGE
 *   None.
 *
 * @FUTURE DIRECTIONS
 *   None.
 *
 * @SEE ALSO
 *   ostr_init(), ostr_init_from_bulk(), ostr_init_from_peer(),
 *   ostr_delete()
 *
 */
int ostr_clear (ostr_t * osp);

/*
 * @NAME
 *   ostr_require_space, ostr_require_more_space
 *   声明存储数据需要的内存空间
 *
 * @SYNOPSIS
 *   #include "octstring.h"
 *   int ostr_require_space (ostr_t * osp, size_t max_size);
 *   int ostr_require_more_space (ostr_t * osp, size_t more_size);
 *
 * @DESCRIPTION
 *   函数 ostr_require_space 声明 ostr_t 内部存储数据所需的内存空间.
 *   函数 ostr_require_more_space 声明以当前 ostr_t 的存储的数据大小上, 内部
 *   存储数据还需要添加的内存空间.
 *   这两个函数被调用后, 如果内部预分配的内存大小不够, 会导致立即重分配内存.
 *
 * @RETURN VALUE
 *   如果成功, 返回零.
 *
 * @ERRORS
 *   None.
 *
 * @EXAMPLES
 *   None.
 *
 * @APPLICATION USAGE
 *   None.
 *
 * @FUTURE DIRECTIONS
 *   None.
 *
 * @SEE ALSO
 *   ostr_init(), ostr_init_from_bulk(), ostr_init_from_peer(),
 *   ostr_create(), ostr_create_from_bulk(), ostr_create_from_peer()
 *   ostr_duplicate(), ostr_delete()
 *
 */
#if OSTRMEM_TRACE
int ostr_require_space_dbg (ostr_t * osp, size_t max_size, const char *filename, int line);
#define ostr_require_space(osp, max_size) ostr_require_space_dbg (osp, max_size, __FILE__, __LINE__)
#else
int ostr_require_space (ostr_t * osp, size_t max_size);
#define ostr_require_space_dbg(osp, max_size, filename, line) ostr_require_space(osp, max_size)
#endif
/*int ostr_require_more_space (ostr_t * osp, size_t more_size); */
#define ostr_require_more_space(osp, more_size) ((NULL == (osp))?(-1):ostr_require_space ((osp), ostr_length(osp) + more_size))

/*
 * @NAME
 *   ostr_length, ostr_total_space, ostr_left_space
 *   获取内部缓冲的数据大小情况
 *
 * @SYNOPSIS
 *   #include "octstring.h"
 *   ssize_t ostr_length (ostr_t * osp);
 *   ssize_t ostr_total_space (ostr_t * osp);
 *   ssize_t ostr_left_space (ostr_t * osp);
 *
 * @DESCRIPTION
 *   函数 ostr_length 获取 ostr_t 内部存储数据的大小.
 *   函数 ostr_total_space 获取 ostr_t 内部存储数据缓冲的大小.
 *   函数 ostr_left_space 获取 ostr_t 内部存储数据缓冲的剩余空间.
 *
 * @RETURN VALUE
 *   如果成功, 返回大小. 失败返回 < 0
 *
 * @ERRORS
 *   None.
 *
 * @EXAMPLES
 *   None.
 *
 * @APPLICATION USAGE
 *   None.
 *
 * @FUTURE DIRECTIONS
 *   None.
 *
 * @SEE ALSO
 *   ostr_init(), ostr_init_from_bulk(), ostr_init_from_peer(),
 *   ostr_create(), ostr_create_from_bulk(), ostr_create_from_peer()
 *   ostr_duplicate(), ostr_delete()
 *
 */
#if 1
ssize_t ostr_length (ostr_t * osp);
ssize_t ostr_total_space (ostr_t * osp);
ssize_t ostr_left_space (ostr_t * osp);
#else
#define ostr_length(osp) ((ssize_t)((NULL == (osp))?(-1):(((ostr_t *)(osp))->size_data)))
#define ostr_left_space(osp)  ((ssize_t)((NULL == (osp))?(-1):(((ostr_t *)(osp))->size_buf - ((ostr_t *)(osp))->size_data)))
#define ostr_total_space(osp) ((ssize_t)((NULL == (osp))?(-1):((ostr_t *)(osp))->size_buf))
#endif // 1

ssize_t ostr_length (ostr_t * osp);
ssize_t ostr_total_space (ostr_t * osp);
ssize_t ostr_left_space (ostr_t * osp);

/*
 * @NAME
 *   ostr_copy_from_bulk, ostr_copy_to_bulk, ostr_copy_from_peer,
 *   ostr_copy_to_peer
 *   数据复制函数
 *
 * @SYNOPSIS
 *   #include "octstring.h"
 *   ssize_t ostr_copy_from_bulk (ostr_t * osp, size_t offset, size_t len,
 *                                opaque_t * bulk);
 *   ssize_t ostr_copy_to_bulk (ostr_t * osp, size_t offset, size_t len,
 *                              opaque_t * bulk);
 *   ssize_t ostr_copy_from_peer (ostr_t * osp, size_t offset, size_t len,
 *                                ostr_t * peer, size_t peer_offset);
 *   ssize_t ostr_copy_to_peer (ostr_t * osp, size_t offset, size_t len,
 *                                ostr_t * peer, size_t peer_offset);
 *
 * @DESCRIPTION
 *
 * @RETURN VALUE
 *   如果成功, 返回大小. 失败返回 < 0
 *
 * @ERRORS
 *   None.
 *
 * @EXAMPLES
 *   None.
 *
 * @APPLICATION USAGE
 *   None.
 *
 * @FUTURE DIRECTIONS
 *   None.
 *
 * @SEE ALSO
 *   ostr_init(), ostr_init_from_bulk(), ostr_init_from_peer(),
 *   ostr_create(), ostr_create_from_bulk(), ostr_create_from_peer()
 *   ostr_duplicate(), ostr_delete()
 *
 */
#if OSTRMEM_TRACE
ssize_t ostr_copy_from_bulk_dbg (ostr_t * osp, size_t offset, size_t len, opaque_t * bulk, const char *filename, int line);
#define ostr_copy_from_bulk(osp, offset, len, bulk) ostr_copy_from_bulk_dbg (osp, offset, len, bulk, __FILE__, __LINE__)
ssize_t ostr_copy_from_peer_dbg (ostr_t * osp, size_t offset, size_t len, ostr_t * peer, size_t peer_offset, const char *filename, int line);
#define ostr_copy_from_peer(osp, offset, len, peer, peer_offset) ostr_copy_from_peer_dbg(osp, offset, len, peer, peer_offset, __FILE__, __LINE__)
ssize_t ostr_copy_to_bulk (ostr_t * osp, size_t offset, size_t len,
                           opaque_t * bulk);

#else
#define ostr_copy_from_bulk_dbg(osp, offset, len, bulk, f, l) ostr_copy_from_bulk(osp, offset, len, bulk)
#define ostr_copy_from_peer_dbg(osp, offset, len, peer, peer_offset, f, l) ostr_copy_from_peer(osp, offset, len, peer, peer_offset)

ssize_t ostr_copy_from_bulk (ostr_t * osp, size_t offset, size_t len,
                             opaque_t * bulk);

ssize_t ostr_copy_to_bulk (ostr_t * osp, size_t offset, size_t len,
                           opaque_t * bulk);
ssize_t ostr_copy_from_peer (ostr_t * osp, size_t offset, size_t len,
                             ostr_t * peer, size_t peer_offset);
#endif

#define ostr_copy_to_peer(osp, offset, len, peer, peer_offset) \
    ostr_copy_from_peer(peer, (size_t)(peer_offset), (size_t)(len), osp, (size_t)(offset))
/*ssize_t ostr_copy_to_peer (ostr_t * osp, size_t offset, size_t len, ostr_t * peer, size_t peer_offset); */

/* 将一个ostr_t中的数据挪到对方，既是buf被置换到peer, 原来的ostr_t变空 */
int ostr_transfer (ostr_t * osp_to, ostr_t * osp_from);
#define ostr_moveto_peer(osp, peer) ostr_transfer ((peer), (osp))

/*
 * @NAME
 *   ostr_append_from_bulk, ostr_append_from_peer, ostr_prepend_from_bulk,
 *   ostr_prepend_from_peer, ostr_insert_from_bulk, ostr_insert_from_peer,
 *   ostr_append_values
 *   数据添加函数
 *
 * @SYNOPSIS
 *   #include "octstring.h"
 *   ssize_t ostr_append_from_bulk (ostr_t * osp, size_t len, opaque_t * bulk);
 *   ssize_t ostr_append_from_peer (ostr_t * osp, size_t len, ostr_t * peer,
 *                                  size_t peer_offset);
 *   ssize_t ostr_prepend_from_bulk (ostr_t * osp, size_t len, opaque_t * bulk);
 *   ssize_t ostr_prepend_from_peer (ostr_t * osp, size_t len, ostr_t * peer,
 *                                   size_t peer_offset);
 *   ssize_t ostr_insert_from_bulk (ostr_t * osp, size_t offset, size_t len,
 *                                  opaque_t * bulk);
 *   ssize_t ostr_insert_from_peer (ostr_t * osp, size_t offset, size_t len,
 *                                  ostr_t * peer, size_t peer_offset);
 *   ssize_t ostr_append_values (ostr_t * op, size_t size, opaque_t value);
 *
 * @DESCRIPTION
 *
 * @RETURN VALUE
 *   如果成功, 返回大小. 失败返回 < 0
 *
 * @ERRORS
 *   None.
 *
 * @EXAMPLES
 *   None.
 *
 * @APPLICATION USAGE
 *   None.
 *
 * @FUTURE DIRECTIONS
 *   None.
 *
 * @SEE ALSO
 *   ostr_init(), ostr_init_from_bulk(), ostr_init_from_peer(),
 *   ostr_create(), ostr_create_from_bulk(), ostr_create_from_peer()
 *   ostr_duplicate(), ostr_delete()
 *
 */

#if 1
ssize_t ostr_append_from_bulk (ostr_t * osp, size_t len, opaque_t * bulk);
ssize_t ostr_append_from_peer (ostr_t * osp, size_t len, ostr_t * peer, size_t peer_offset);
#else
#define ostr_append_from_bulk(osp, len, bulk) ((NULL == osp)?(-1):ostr_copy_from_bulk ((osp), ostr_length(osp), (len), (opaque_t *)(bulk)))
#if MEMWATCH
#define ostr_append_from_peer(osp, len, peer, peer_offset) ((NULL == osp)?(-1):ostr_copy_from_peer_dbg ((osp), ostr_length(osp), (len), (peer), (peer_offset), (__FILE__), (__LINE__)))
#else
#define ostr_append_from_peer(osp, len, peer, peer_offset) ((NULL == osp)?(-1):ostr_copy_from_peer ((osp), ostr_length(osp), (len), (peer), (peer_offset)))
#endif
#endif // 1

ssize_t ostr_append_values (ostr_t * op, size_t size, opaque_t value);

/*ssize_t ostr_prepend_from_bulk (ostr_t * osp, size_t len, opaque_t * bulk); */
#define ostr_prepend_from_bulk(osp, len, bulk) ((NULL == osp)?(-1):ostr_insert_from_bulk ((osp), 0, (len), (bulk)))
/*ssize_t ostr_prepend_from_peer (ostr_t * osp, size_t len, ostr_t * peer, size_t peer_offset); */
#define ostr_prepend_from_peer(osp, len, peer, peer_offset) ((NULL == osp)?(-1):ostr_insert_from_peer ((osp), 0, (len), (peer), (peer_offset)))

ssize_t ostr_insert_from_bulk (ostr_t * osp, size_t offset, size_t len,
                               opaque_t * bulk);
ssize_t ostr_insert_from_peer (ostr_t * osp, size_t offset, size_t len,
                               ostr_t * peer, size_t peer_offset);

/*
 * @NAME
 *   ostr_truncate, ostr_delete
 *   数据删除函数
 *
 * @SYNOPSIS
 *   #include "octstring.h"
 *   ssize_t ostr_truncate (ostr_t * osp, size_t newlen);
 *   ssize_t ostr_delete (ostr_t * osp, size_t offset, size_t len);
 *
 * @DESCRIPTION
 *
 * @RETURN VALUE
 *   如果成功, 返回大小. 失败返回 < 0
 *
 * @ERRORS
 *   None.
 *
 * @EXAMPLES
 *   None.
 *
 * @APPLICATION USAGE
 *   None.
 *
 * @FUTURE DIRECTIONS
 *   None.
 *
 * @SEE ALSO
 *   ostr_init(), ostr_init_from_bulk(), ostr_init_from_peer(),
 *   ostr_create(), ostr_create_from_bulk(), ostr_create_from_peer()
 *   ostr_duplicate(), ostr_delete()
 *
 */
ssize_t ostr_truncate (ostr_t * osp, size_t newlen);
ssize_t ostr_delete (ostr_t * osp, size_t offset, size_t len);
void ostr_free4reserve (ostr_t * osp, size_t newlen); /* 建议尽量保留 newlen 的字节，如果原内存太大，则会释放掉并置内部缓存为空。用于程序使用后返回 */

/*int ostr_getchar (ostr_t * osp, size_t offset, char *ret_char); */
/*int ostr_getchar_by_ret (ostr_t * osp, size_t offset); */
#define ostr_getchar(osp, offset, ret_char) ostr_copy_to_bulk (osp, offset, 1, (opaque_t *)(ret_char))
int ostr_putchar (ostr_t * osp, size_t offset, unsigned char ch);
/* 获取内部数据的指针 */
#if 1
opaque_t * ostr_getref_data(ostr_t * osp, size_t offset, size_t len);
#else
#define ostr_getref_data(osp, offset, len) (((offset) + (len) > ostr_length (osp))?NULL:(osp)->buf + (offset))
#endif
#define ostr_getchar_by_ret(osp, offset) (*((char *)ostr_getref_data(osp, offset, 1)))

/*
 * @NAME
 *   ostr_search_bulk, ostr_search_peer
 *   ostr_casecstr_bulk, ostr_casecstr_peer
 *   查找函数
 *
 * @SYNOPSIS
 *   #include "octstring.h"
 *   ssize_t ostr_search_bulk (ostr_t * osp, size_t offset_base, const opaque_t *needle, size_t sz_neddle);
 *   ssize_t ostr_search_peer (ostr_t * osp_base, size_t offset_base, const ostr_t *needle);
 *   ssize_t ostr_casecstr_bulk (ostr_t * osp, size_t offset_base, const opaque_t *needle, size_t sz_neddle);
 *   ssize_t ostr_casecstr_peer (ostr_t * osp_base, size_t offset_base, const ostr_t *needle);
 *
 * @DESCRIPTION
 *
 * @RETURN VALUE
 *   如果成功, 返回位置. 失败返回 < 0
 *
 * @ERRORS
 *   None.
 *
 * @EXAMPLES
 *   None.
 *
 * @APPLICATION USAGE
 *   None.
 *
 * @FUTURE DIRECTIONS
 *   None.
 *
 * @SEE ALSO
 *   ostr_init(), ostr_init_from_bulk(), ostr_init_from_peer(),
 *   ostr_create(), ostr_create_from_bulk(), ostr_create_from_peer()
 *   ostr_duplicate(), ostr_delete()
 *
 */
ssize_t ostr_search_bulk (ostr_t * osp, size_t offset_base, const opaque_t *needle, size_t sz_neddle);
/*ssize_t ostr_search_peer (ostr_t * osp_base, size_t offset_base, const ostr_t *needle); */
#define ostr_search_peer(osp_base, offset_base, needle) ostr_search_bulk (osp_base, offset_base, ostr_getref_data(needle, 0, ostr_length(needle)), ostr_length(needle))
ssize_t ostr_casecstr_bulk (ostr_t * osp, size_t offset_base, const opaque_t *needle, size_t sz_neddle);
/*ssize_t ostr_casecstr_peer (ostr_t * osp_base, size_t offset_base, const ostr_t *needle); */
#define ostr_casecstr_peer(osp_base, offset_base, needle) ostr_casecstr_bulk (osp_base, offset_base, ostr_getref_data(needle, 0, ostr_length(needle)), ostr_length(needle))

/*
 * @NAME
 *   ostr_memset, ostr_memcmp_bulk, ostr_memcmp_peer
 *   数据区域操作函数
 *
 * @SYNOPSIS
 *   #include "octstring.h"
 *   int ostr_memset (ostr_t * osp, size_t offset, size_t len, opaque_t value);
 *   int ostr_memcmp_bulk (ostr_t * osp, size_t offset, size_t len, opaque_t * bulk);
 *   int ostr_memcmp_peer (ostr_t * osp, size_t offset, size_t len, ostr_t * peer,
 *                         size_t peer_offset);
 *
 * @DESCRIPTION
 *
 * @RETURN VALUE
 *   ostr_memset() 返回 osp. ostr_memcmp_bulk() 和 ostr_memcmp_peer 返回
 *   -1 (left < right), 0 (left == right), 1 (left > right)
 *
 * @ERRORS
 *   None.
 *
 * @EXAMPLES
 *   None.
 *
 * @APPLICATION USAGE
 *   None.
 *
 * @FUTURE DIRECTIONS
 *   None.
 *
 * @SEE ALSO
 *   ostr_init(), ostr_init_from_bulk(), ostr_init_from_peer(),
 *   ostr_create(), ostr_create_from_bulk(), ostr_create_from_peer()
 *   ostr_duplicate(), ostr_delete()
 *
 */
int ostr_memset (ostr_t * osp, size_t offset, size_t len, opaque_t value);
int ostr_memcmp_bulk (ostr_t * osp, size_t offset, size_t len,
                      opaque_t * bulk);
int ostr_memcmp_peer (ostr_t * osp, size_t offset, size_t len,
                      ostr_t * peer, size_t peer_offset, size_t peer_len);
int ostr_memcasecmp_bulk (ostr_t * osp, size_t offset, size_t len, opaque_t * bulk);

typedef ssize_t (* ostr_cb_fsys_read_t) (void *fd, void * buffer, size_t size);
#define ostr_cb_fsys_write_t ostr_cb_fsys_read_t
ssize_t ostr_write_callback (ostr_t * osp, size_t offset, size_t len, void *fd, ostr_cb_fsys_write_t cb_write);
ssize_t ostr_read_callback (ostr_t * osp, size_t offset_osp, size_t len, void *fd, ostr_cb_fsys_read_t cb_read);

ssize_t ostr_read_fd (ostr_t * osp, size_t offset_osp, size_t len, int fd);
ssize_t ostr_write_fd (ostr_t * osp, size_t offset_osp, size_t len, int fd);

ssize_t ostr_read_stream (ostr_t * osp, size_t offset_osp, size_t len, FILE * fp);
/*#define ostr_read_stream(osp, offset, len, fp) ((NULL == fp)?(-1):ostr_read_fd ((osp), (offset), (len), fileno(fp))) */
ssize_t ostr_write_stream (ostr_t * osp, size_t offset_osp, size_t len, FILE *fp);
/*#define ostr_write_stream(osp, offset, len, fp) ((NULL == osp)?(-1):ostr_write_fd ((osp), (offset), (len), fileno(fp))) */

/*int ostr_get_bits(ostr_t *osp, size_t bit_offset, size_t numbits, opaque_t * bulk); */
/*int ostr_set_bits(ostr_t *osp, size_t bit_offset, size_t numbits, opaque_t * bulk); */

/*
 * @NAME
 *   ostr_format, ostr_format_append, ostr_format_new
 *   数据格式化函数
 *
 * @SYNOPSIS
 *   #include "octstring.h"
 *   ssize_t ostr_format (ostr_t * osp, size_t offset, const char *fmt, ...);
 *   ssize_t ostr_format_append (ostr_t * osp, const char *fmt, ...);
 *   ostr_t *ostr_format_new (const char *fmt, ...);
 *
 * @DESCRIPTION
 *
 * @RETURN VALUE
 *   对于函数 ostr_format() 和 ostr_format_append, 如果成功, 返回格式化字串长度
 *   出错返回-1
 *
 *   对于函数 ostr_format_new(), 返回一个新的存有格式化字串的 ostr_t 结构指针.
 *   出错返回 NULL
 *
 * @ERRORS
 *   None.
 *
 * @EXAMPLES
 *   None.
 *
 * @APPLICATION USAGE
 *   None.
 *
 * @FUTURE DIRECTIONS
 *   None.
 *
 * @SEE ALSO
 *   ostr_init(), ostr_init_from_bulk(), ostr_init_from_peer(),
 *   ostr_create(), ostr_create_from_bulk(), ostr_create_from_peer()
 *   ostr_duplicate(), ostr_delete()
 *
 */
ssize_t ostr_format (ostr_t * osp, size_t offset, const char *fmt, ...);
ssize_t ostr_format_append (ostr_t * osp, const char *fmt, ...);
ostr_t *ostr_format_new (const char *fmt, ...);

/*
 * @NAME
 *   ostr_pack_with_length, ostr_pack_with_length_new
 *   数据打包函数
 *
 * @SYNOPSIS
 *   #include "octstring.h"
 *   ssize_t ostr_pack_with_length (ostr_t * osp, size_t offset, size_t len,
 *                                  int num_bytes_len, ostr_t * peer,
 *                                  size_t peer_offset);
 *   ostr_t *ostr_pack_with_length_new (ostr_t * osp, size_t offset, size_t len,
 *                                      int num_bytes_len);
 *
 * @DESCRIPTION
 *
 * @RETURN VALUE
 *   对于函数 ostr_pack_with_length, 如果成功, 返回格式化字串长度, 出错返回 -1
 *
 *   对于函数 ostr_pack_with_length_new, 返回一个新的存有格式化字串的 ostr_t
 *   结构指针. 出错返回 NULL
 *
 * @ERRORS
 *   None.
 *
 * @EXAMPLES
 *   None.
 *
 * @APPLICATION USAGE
 *   None.
 *
 * @FUTURE DIRECTIONS
 *   None.
 *
 * @SEE ALSO
 *   ostr_init(), ostr_init_from_bulk(), ostr_init_from_peer(),
 *   ostr_create(), ostr_create_from_bulk(), ostr_create_from_peer()
 *   ostr_duplicate(), ostr_delete()
 *
 */
ssize_t ostr_pack_with_length (ostr_t * osp, size_t offset, size_t len,
                               int num_bytes_len, ostr_t * peer,
                               size_t peer_offset);
ostr_t *ostr_pack_with_length_new (ostr_t * osp, size_t offset,
                                   size_t len, int num_bytes_len);

/* 一个可以将数据以HEX打印出来到任何缓冲中的函数，可以用于程序调试中打印数据 */
int ostr_cb_writer_fdwrite (void *fd, opaque_t * fragment, size_t size);
typedef int (*bhd_cb_writer_t) (void *fd, opaque_t * fragment, size_t size);
size_t bulk_hex_dump (void *fd, opaque_t * fragment, size_t size, bhd_cb_writer_t writer, int use_c_style);
#define hex_dump_to_fd(fd, fragment, size) bulk_hex_dump ((void *)(fd), fragment, size, ostr_cb_writer_fdwrite, 1)
ssize_t ostr_dump_seg_to_fd (ostr_t * osp, const char *msg, size_t startpos, size_t max_size, int fd);
ssize_t ostr_dump_seg_to_callback (ostr_t * osp, const char *msg, size_t startpos, size_t max_size, bhd_cb_writer_t writer, void *fd);

/* 将 ostr_t 结构中的数据信息打印到指定的地方 */
ssize_t ostr_dump_to_bulk (ostr_t * osp, size_t max_size, opaque_t * bulk);
ssize_t ostr_dump_to_fd (ostr_t * osp, size_t max_size, int fd);
ssize_t ostr_dump_to_peer (ostr_t * osp, ostr_t * osp_peer);

/*#define ostr_dump_to_stream(osp, maxsize, fp) (0) //printf ("unable to dump the octstring!\n") */
//#define ostr_dump_to_stream(osp, maxsize, fp) ((NULL == osp)?(-1):ostr_dump_to_fd ((osp), (maxsize), fileno(fp)))
ssize_t ostr_dump_to_stream (ostr_t * osp, size_t max_size, FILE *fp);

int ostr_getval_uint32 (ostr_t *osp, size_t pos, uint32_t * val_ret);

#define OSTR_APPEND_STATIC_CSTR(osp, cstr) ostr_append_from_bulk ((osp), sizeof (cstr) - 1, (opaque_t *)(cstr))
#define OSTR_COPYFROM_STATIC_CSTR(osp, offset_osp, cstr) ostr_copy_from_bulk((osp), offset_osp, sizeof (cstr) - 1, (opaque_t *)(cstr))
#define OSTR_SEARCH_STATIC_CSTR(osp, offset_base, needle) ostr_search_bulk((osp), (offset_base), (opaque_t *)(needle), sizeof (needle) - 1)

#define OSTR_APPEND_STATIC_WSTR(osp, wstr) ostr_append_from_bulk ((osp), sizeof (wstr) - sizeof(wchar_t), (opaque_t *)(wstr))

extern int memcasecmp (const char * buf1, const char * buf2, size_t szbuf);


#ifdef __cplusplus
}
#endif

#endif
