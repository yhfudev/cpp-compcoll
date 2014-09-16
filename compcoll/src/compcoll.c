/**
 * @file    compcoll.c
 * @brief   Collate the contents by comparing two versions of the texts
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2014-09-07
 */
#define _GNU_SOURCE 1
#include <stdint.h>    /* uint8_t */
#include <stdlib.h>    /* size_t */
#include <unistd.h> // write()
#include <sys/types.h> /* ssize_t */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

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

#include "getline.h"

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG
#define USE_OUT_ED_TABLE 1

#define TRACE(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define DBGMSG(catlog, level, fmt, ...) fprintf (stderr, "[%s()]\t" fmt "\t{%d," __FILE__ "}\n", __func__, ##__VA_ARGS__, __LINE__)
#else
#define USE_OUT_ED_TABLE 0
#define TRACE(...)
#define DBGMSG(...)
#endif

#undef USE_OUT_ED_TABLE
#define USE_OUT_ED_TABLE 0


/**********************************************************************************/

#define VER_MAJOR 0
#define VER_MINOR 1
#define VER_MOD   1

static void
version (void)
{
    fprintf (stderr, "Collate the contents by comparing two versions of the texts\n");
    fprintf (stderr, "Version %d.%d.%d\n", VER_MAJOR, VER_MINOR, VER_MOD);
    fprintf (stderr, "Copyright (c) 2014 Y. Fu. All rights reserved.\n\n");
}

static void
help (char *progname)
{
    fprintf (stderr, "Usage: \n"
        "\t%s <old file> <new file>\n"
        , basename(progname));
    fprintf (stderr, "\nOptions:\n");
    //fprintf (stderr, "\t-p <port #>\tthe listen port\n");
    fprintf (stderr, "\t-H\tshow the HTML header\n");
    fprintf (stderr, "\t-T\tshow the HTML tail\n");
    fprintf (stderr, "\t-C\tshow the HTML content only(no HTML header and tail)\n");
    fprintf (stderr, "\t-h\tPrint this message.\n");
    fprintf (stderr, "\t-v\tVerbose information.\n");
}

static void
usage (char *progname)
{
    version ();
    help (progname);
}



/**********************************************************************************/
/**
 * @brief 查找下个 UTF-8 字符的位置
 *
 * @param pstart : 存储 UTF-8 字符的指针
 *
 * @return 成功返回下个 UTF-8 字符的位置
 *
 * 查找下个 UTF-8 字符的位置
 */
uint8_t *
get_next_utf8_pos (uint8_t *pstart)
{
    uint8_t *p = pstart;
    if (0 == (0x80 & *p)) {
        return p + 1;
    }
    if (0x80 == (0xC0 & *p)) {
        for (; 0x80 == (0xC0 & *p); p ++);
        return p;
    }
    if (0xC0 == (0xE0 & *p)) {
        return p + 2;
    }
    if (0xE0 == (0xF0 & *p)) {
        return p + 3;
    }
    if (0xF0 == (0xF8 & *p)) {
        return p + 4;
    }
    if (0xF8 == (0xFC & *p)) {
        return p + 5;
    }
    return p + 6;
    /*if (0xFC == (0xFE & *p)) {
        return p + 6;
    } */
}

wchar_t
get_val_utf82uni (uint8_t *pstart)
{
    size_t cntleft;
    wchar_t retval = 0;

    if (0 == (0x80 & *pstart)) {
        return *pstart;
    }

    if (((*pstart & 0xE0) ^ 0xC0) == 0) {
        cntleft = 1;
        retval = *pstart & ~0xE0;
    } else if (((*pstart & 0xF0) ^ 0xE0) == 0) {
        cntleft = 2;
        retval = *pstart & ~0xF0;
    } else if (((*pstart & 0xF8) ^ 0xF0) == 0) {
        cntleft = 3;
        retval = *pstart & ~0xF8;
    } else if (((*pstart & 0xFC) ^ 0xF8) == 0) {
        cntleft = 4;
        retval = *pstart & ~0xFC;
    } else if (((*pstart & 0xFE) ^ 0xFC) == 0) {
        cntleft = 5;
        retval = *pstart & ~0xFE;
    } else {
        /* encoding error */
        cntleft = 0;
        retval = 0;
    }
    pstart ++;
    for (; cntleft > 0; cntleft --) {
        retval <<= 6;
        retval |= *pstart & 0x3F;
        pstart ++;
    }
    return retval;
}

/**
 * @brief 转换 UTF-8 编码的一个字符为本地的 Unicode 字符(wchar_t)
 *
 * @param pstart : 存储 UTF-8 字符的指针
 * @param pval : 需要返回的 Unicode 字符存放地址指针
 *
 * @return 成功返回下个 UTF-8 字符的位置
 *
 * 转换 UTF-8 编码的一个字符为本地的 Unicode 字符(wchar_t)
 */
uint8_t *
get_utf8_value (uint8_t *pstart, wchar_t *pval)
{
    uint32_t val = 0;
    uint8_t *p = pstart;
    /*size_t maxlen = strlen (pstart);*/

    assert (NULL != pstart);

    if (0 == (0x80 & *p)) {
        val = (size_t)*p;
        p ++;
    } else if (0xC0 == (0xE0 & *p)) {
        val = *p & 0x1F;
        val <<= 6;
        p ++;
        val |= (*p & 0x3F);
        p ++;
        assert ((wchar_t)val == get_val_utf82uni (pstart));
    } else if (0xE0 == (0xF0 & *p)) {
        val = *p & 0x0F;
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        p ++;
        assert ((wchar_t)val == get_val_utf82uni (pstart));
    } else if (0xF0 == (0xF8 & *p)) {
        val = *p & 0x07;
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        p ++;
        assert ((wchar_t)val == get_val_utf82uni (pstart));
    } else if (0xF8 == (0xFC & *p)) {
        val = *p & 0x03;
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        p ++;
        assert ((wchar_t)val == get_val_utf82uni (pstart));
    } else if (0xFC == (0xFE & *p)) {
        val = *p & 0x01;
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        p ++;
        assert ((wchar_t)val == get_val_utf82uni (pstart));
    } else if (0x80 == (0xC0 & *p)) {
        /* error? */
        for (; 0x80 == (0xC0 & *p); p ++);
    } else {
        /* error */
        for (; ((0xFE & *p) > 0xFC); p ++);
    }
    /*if (val == 0) {
        p = NULL;*/
/*
    } else if (pstart + maxlen < p) {
        p = pstart;
        if (pval) *pval = 0;
    }
*/

    if (pval) *pval = val;

    return p;
}


/**
 * @brief 转换本地的 Unicode 的一个字符(wchar_t) 为 UTF-8 编码
 *
 * @param val : 需要转换的 Unicode 字符值
 * @param buf : 存储 UTF-8 字符的缓冲
 * @param szbuf : 缓冲大小
 *
 * @return 成功返回0
 *
 * 转换本地的 Unicode 的一个字符(wchar_t) 为 UTF-8 编码, 结果保存到 osp 后面
 *
 * The UTF-FSS (aka UTF-2) encoding of UCS, as described in the following
 * quote from Ken Thompson's utf-fss.c:
 *
 * Bits  Hex Min  Hex Max  Byte Sequence in Binary                            leftChar
 *   7  00000000 0000007f 0vvvvvvv                                              0
 *  11  00000080 000007FF 110vvvvv 10vvvvvv                                     1
 *  16  00000800 0000FFFF 1110vvvv 10vvvvvv 10vvvvvv                            2
 *  21  00010000 001FFFFF 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv                   3
 *  26  00200000 03FFFFFF 111110vv 10vvvvvv 10vvvvvv 10vvvvvv 10vvvvvv          4
 *  31  04000000 7FFFFFFF 1111110v 10vvvvvv 10vvvvvv 10vvvvvv 10vvvvvv 10vvvvvv 5
 *
 * The UCS value is just the concatenation of the v bits in the multibyte
 * encoding.  When there are multiple ways to encode a value, for example
 * UCS 0, only the shortest encoding is legal.
 */
int
uni_to_utf8 (size_t val, uint8_t *buf, size_t szbuf)
{
    size_t mask;
    int i;
    uint8_t *p = buf;
    int ret = 0;

    mask = 0xFFFF0000;
    /* check the bit of value */
    if (0 == (mask & val)) {
        mask = 0xFFFFFFFF;
        i = 0;
        for (; mask & val; mask <<= 1) {
            i ++;
        }
    } else {
        i = 16;
        for (; mask & val; mask <<= 1) {
            i ++;
        }
    }
    if (i < 8) {
        *p = val;
        ret = 1;
    } else if (i < 12) {
        *p = (val >> 6) | 0xC0;
        p ++;
        *p = (val & 0x3F) | 0x80;
        ret = 2;
    } else if (i < 17) {
        *p = (val >> 12) | 0xE0;
        p ++;
        *p = ((val >> 6) & 0x3F) | 0x80;
        p ++;
        *p = (val & 0x3F) | 0x80;
        ret = 3;
    } else if (i < 22) {
        *p = (val >> 18) | 0xF0;
        p ++;
        *p = ((val >> 12) & 0x3F) | 0x80;
        p ++;
        *p = ((val >> 6) & 0x3F) | 0x80;
        p ++;
        *p = (val & 0x3F) | 0x80;
        ret = 4;
    } else if (i < 27) {
        *p = (val >> 24) | 0xF8;
        p ++;
        *p = ((val >> 18) & 0x3F) | 0x80;
        p ++;
        *p = ((val >> 12) & 0x3F) | 0x80;
        p ++;
        *p = ((val >> 6) & 0x3F) | 0x80;
        p ++;
        *p = (val & 0x3F) | 0x80;
        ret = 5;
    } else {
        *p = (val >> 30) | 0xFC;
        p ++;
        *p = ((val >> 24) & 0x3F) | 0x80;
        p ++;
        *p = ((val >> 18) & 0x3F) | 0x80;
        p ++;
        *p = ((val >> 12) & 0x3F) | 0x80;
        p ++;
        *p = ((val >> 6) & 0x3F) | 0x80;
        p ++;
        *p = (val & 0x3F) | 0x80;
        ret = 6;
    }

    return ret;
}



/**********************************************************************************/

// 操作UTF8字串的接口

typedef int (* strcmp_cb_output_t) (void *userdata, FILE *fp, int right, size_t idx); /* idx -- the index of the string; right -- 0 -- `left' string, 1 -- `right' string */

typedef int (* strcmp_cb_comp_t) (void *userdata, size_t idx1, size_t idx2); /* idx1 -- the index of the `left' string; idx2 -- right */
typedef int (* strcmp_cb_length_t) (void *userdata, int right); /* 0 -- `left' string, 1 -- `right' string */
typedef wchar_t (* strcmp_cb_getval_t) (void *userdata, int right, size_t idx);  /* idx -- the index of the string; right -- 0 -- `left' string, 1 -- `right' string */

typedef int (* strcmp_cb_matget_t) (void *userdata, size_t idx1, size_t idx2); /* get the value at (idx1, idx2) */
typedef int (* strcmp_cb_matset_t) (void *userdata, size_t idx1, size_t idx2, int val); /* set the value at (idx1, idx2) */
typedef int (* strcmp_cb_matresize_t) (void *userdata, size_t row, size_t col); /* resize the matrix to (row, col), and clear it */

typedef struct _strcmp_t {
    void * userdata_str;
    strcmp_cb_comp_t   cb_comp;
    strcmp_cb_length_t cb_len;
    strcmp_cb_output_t cb_output;
    strcmp_cb_getval_t cb_getval;

    void * userdata_matrix;
    strcmp_cb_matget_t     cb_matget;
    strcmp_cb_matset_t     cb_matset;
    strcmp_cb_matresize_t  cb_matresz;
    void * userdata_matrix2;
} strcmp_t;

/**********************************************************************************/

#define MIN(a,b) (((a)<(b))?(a):(b))

#define EDIS_NONE   0x00 /*!< No action */
#define EDIS_INSERT 0x01 /*!< Insert action */
#define EDIS_DELETE 0x02 /*!< Delete action */
#define EDIS_REPLAC 0x03 /*!< Replace action */
#define EDIS_IGNORE 0x04 /*!< Ignore action */

const char *
edaction_val2cstr (char val)
{
    switch (val) {
    case EDIS_NONE:
        return ("x");
        break;
    case EDIS_INSERT:
        return ("S");
        break;
    case EDIS_DELETE:
        return ("D");
        break;
    case EDIS_REPLAC:
        return ("R");
        break;
    case EDIS_IGNORE:
        return ("I");
        break;
    }
    return "?";
}

/**
 * @brief 计算两个字符串的距离
 *
 * @param stra : 第1个字符串
 * @param lena : 第1个字符串的长度
 * @param strb : 第2个字符串
 * @param lenb : 第2个字符串的长度
 *
 * @return 返回距离值
 *
 * 本函数editdistance(A[1..m],B[1..n])只返回距离值，没有修改路径。时间O(m*n),空间O(min(m,n))
 *
 * stra 变成 strb 所需要做的步骤
 *
 * string A: about
 * string B: fout
 * string A => string B
 *
 * --> A
 * |
 * V
 * B
 *  | |a|b|o|u|t|
 * -+-+-+-+-+-+-+
 *  |0|1|2|3|4|5|
 * -+-+-+-+-+-+-+
 * f|1| | | | | |
 * -+-+-+-+-+-+-+
 * o|2| | | | | |
 * -+-+-+-+-+-+-+
 * u|3| | | | | |
 * -+-+-+-+-+-+-+
 * t|4| | | | | |
 * -+-+-+-+-+-+-+
 * 观察以上表格可以知道, 如果不关心编辑路径, 只需要长度结果的话:
 * 1. 可以循环以行来计算各个空格中的数据；因为各个空格的值是由 上(Insert), 左(Delete), 斜上(Ignore/Replace) 这三个位置的值决定的。
 * 2. 对于 Edit distance 这个问题对应的 matrix, 初始0行／列上的各个位置的值都是固定的。当以循环以行来计算时，这些值可以通过位置推算出。
 *
 * 所以，我们可以获得比 O(m*n) 和 O(m+n) 更进一步的空间需求优化： O(min(m,n))，而运行时间还是 O(m*n)
 */
/*
the edit distance between $a = a_1\ldots a_n$ and $b = b_1\ldots b_m$ is given by $d_{mn}$,
defined by the recurrence
$$
d_{i0} = \sum_{k=1}^{i} w_\mathrm{del}(b_{k}), \quad  for\; 1 \leq i \leq m

d_{0j} = \sum_{k=1}^{j} w_\mathrm{ins}(a_{k}), \quad  for\; 1 \leq j \leq n

d_{ij} = \begin{cases} d_{i-1, j-1} & \quad a_{j} = b_{i}\\ \min \begin{cases} d_{i-1, j} + w_\mathrm{del}(b_{i})\\ d_{i,j-1} + w_\mathrm{ins}(a_{j}) \\ d_{i-1,j-1} + w_\mathrm{sub}(a_{j}, b_{i}) \end{cases} & \quad a_{j} \neq b_{i}\end{cases} , \quad  for\; 1 \leq i \leq m, 1 \leq j \leq n.
$$
*/
int
ed_edit_distance (strcmp_t *cmpinfo)
{
    //static int *g_matrix = NULL;
    //static int g_num_matrix = 0;

    int i;
    int j;

    int pi; /* Pinsert */
    int pd; /* Pdelete */
    int pr; /* Pignore/Preplace */
    int crossval; /* 斜临的值 */

    assert (NULL != cmpinfo);
    assert (NULL != cmpinfo->cb_comp);
    assert (NULL != cmpinfo->cb_len);
    assert (NULL != cmpinfo->cb_matget);
    assert (NULL != cmpinfo->cb_matset);
    assert (NULL != cmpinfo->cb_matresz);
#if USE_OUT_ED_TABLE
    assert (NULL != cmpinfo->cb_output);
    assert (NULL != cmpinfo->cb_getval);
#endif

    // 如果有一个字符串的长度为0,则直接返回值
    if (cmpinfo->cb_len(cmpinfo->userdata_str, 0) < 1) {
        return cmpinfo->cb_len(cmpinfo->userdata_str, 1);
    }
    if (cmpinfo->cb_len(cmpinfo->userdata_str, 1) < 1) {
        return cmpinfo->cb_len(cmpinfo->userdata_str, 0);
    }
//#if ! DEBUG
    ///* swap the array with minmal number of items to the vertical line */
    //if (lena > lenb) {
        //char * tmp = (char *)stra;
        //stra = strb;
        //strb = tmp;
        //i = lena;
        //lena = lenb;
        //lenb = i;
    //}
//#endif

    // 设置行缓冲
    cmpinfo->cb_matresz (cmpinfo->userdata_matrix, 1, cmpinfo->cb_len(cmpinfo->userdata_str, 0) + 1);

#if USE_OUT_ED_TABLE
    // 用于记录操作, 只做显示用
    char action = EDIS_NONE;
    cmpinfo->cb_matresz (cmpinfo->userdata_matrix2, 1, cmpinfo->cb_len(cmpinfo->userdata_str, 0) + 1);

    TRACE ("    |    |");
    for (i = 0; i < cmpinfo->cb_len(cmpinfo->userdata_str, 0); i ++) {
        TRACE (" ");
        cmpinfo->cb_output (cmpinfo->userdata_str, stderr, 0, i);
        TRACE (" |");
    }
    TRACE ("\n");
    TRACE ("    |");
#endif
    for (i = 0; i <= cmpinfo->cb_len(cmpinfo->userdata_str, 0); i ++) {
        cmpinfo->cb_matset (cmpinfo->userdata_matrix, 0, i, i);
#if USE_OUT_ED_TABLE
        TRACE (" %2d |", i);
#endif
    }
#if USE_OUT_ED_TABLE
    TRACE ("\n");
#endif
    for (i = 1; i <= cmpinfo->cb_len(cmpinfo->userdata_str, 1); i ++) {
        crossval = i - 1;
#if USE_OUT_ED_TABLE
        TRACE (" ");
        cmpinfo->cb_output (cmpinfo->userdata_str, stderr, 1, i-1);
        TRACE (" |");
        TRACE (" %2d |", i);
#endif
        for (j = 1; j <= cmpinfo->cb_len(cmpinfo->userdata_str, 0); j ++) {
#if USE_OUT_ED_TABLE
            action = EDIS_NONE;
#endif
            /* matrix[i-1][j] + 1 */
            pi = 1 + cmpinfo->cb_matget (cmpinfo->userdata_matrix, 0, j); /* 之前的一行刷新后不再需要了，所以可以重复使用这行的缓冲 */

            /* matrix[i][j-1] + 1 */
            if (j == 1) {
                pd = i + 1; /* 因为在这0列上的值是固定的 */
            } else {
                //pd = g_matrix[j - 1] + 1;
                pd = 1 + cmpinfo->cb_matget (cmpinfo->userdata_matrix, 0, j - 1);
            }

            /* matrix[i-1][j-1] + (B[i-1]!=A[j-1]) */
            //pr = crossval + (strb[i - 1] == stra[j - 1]?0:1);
            pr = crossval + ((0 == cmpinfo->cb_comp(cmpinfo->userdata_str, j - 1, i - 1))?0:1);

#if USE_OUT_ED_TABLE
            if (pi > pd) {
                action = EDIS_DELETE;
            } else {
                action = EDIS_INSERT;
            }
#endif
            pi = MIN (pi, pd);

#if USE_OUT_ED_TABLE
            if (pi > pr) {
                action = EDIS_REPLAC;
            }
#endif
            //crossval = g_matrix[j];
            crossval = cmpinfo->cb_matget (cmpinfo->userdata_matrix, 0, j);  /* crossval 存储左上角的数据值，因为前一行的数据将被冲掉，该值是唯一需要保存的 */
            //g_matrix[j] = MIN (pi, pr);
            cmpinfo->cb_matset (cmpinfo->userdata_matrix, 0, j, MIN (pi, pr));
#if USE_OUT_ED_TABLE
            cmpinfo->cb_matset (cmpinfo->userdata_matrix2, 0, j, action);
            TRACE (" %2d |", cmpinfo->cb_matget (cmpinfo->userdata_matrix, 0, j));
#endif
        }
#if USE_OUT_ED_TABLE
        TRACE ("\n");
        TRACE ("----|----|");
        for (j = 1; j <= cmpinfo->cb_len(cmpinfo->userdata_str, 0); j ++) {
            TRACE ("^ ");
            TRACE ("%s", edaction_val2cstr (cmpinfo->cb_matget (cmpinfo->userdata_matrix2, 0, j)));
            TRACE ("^|");
        }
        TRACE ("\n");
#endif
    }
    //return g_matrix[cmpinfo->cb_len(cmpinfo->userdata_str, 0)];
    return cmpinfo->cb_matget (cmpinfo->userdata_matrix, 0, cmpinfo->cb_len(cmpinfo->userdata_str, 0) );
}

/**
 * @brief 计算两个字符串的距离
 *
 * @param stra : 第1个字符串
 * @param lena : 第1个字符串的长度
 * @param strb : 第2个字符串
 * @param lenb : 第2个字符串的长度
 * @param path : 修改路径
 * @param ret_numpath : 修改路径缓冲长度
 *
 * @return 返回距离值
 *
 * 本函数editdistance(A[1..m],B[1..n])返回距离值和修改路径。时间O(m*n),空间O(m*n)
 *    the buffer of the path should be >= strlen(stra)+strlen(strb)
 *    each value of the item of the path is type char and one of three value: DEL(0x01) the item of string a, INS(0x02), REPL(0x03), NON(0x00)
 */
int
ed_edit_distance_path (strcmp_t *cmpinfo, char *path, size_t *ret_numpath)
{
    /* 这里函数需要返回编辑路径。在计算出最后终点的值后需要追踪返回的路径，此时需要系统保留所有位置的变化情况。
       因为在得到最终结果前，不知道究竟终点值是从哪条路径得到的。
       在各个点上的计算出的值很多都是相同的，所以不可能知道究竟哪个点是最优路径经过的。
       因此，空间需求只能是 O(m*n) */
    //static int *g_matrix_val = NULL;  /* 存储当前值 */
    //static char *g_matrix_dir = NULL; /* 存储路径方向 */
    //static int g_num_matrix = 0;

    int i;
    int j;

    int pi; /* Pinsert */
    int pd; /* Pdelete */
    int pr; /* Pignore/Preplace */
    char flg_equ;
    int lena;
    int lenb;

    if (NULL == path) {
        return ed_edit_distance (cmpinfo);
    }
    assert (NULL != path);
    assert (NULL != ret_numpath);
    lena = cmpinfo->cb_len(cmpinfo->userdata_str, 0);
    lenb = cmpinfo->cb_len(cmpinfo->userdata_str, 1);
    assert (lena >= 0);
    assert (lena >= 0);
    if (lena < 1) {
        for (i = 0; i < lenb; i ++) {
            path[i] = EDIS_INSERT;
        }
        return lenb;
    }
    if (lenb < 1) {
        for (i = 0; i < lena; i ++) {
            path[i] = EDIS_INSERT;
        }
        return lena;
    }

    // 设置缓冲
    cmpinfo->cb_matresz (cmpinfo->userdata_matrix, lenb + 1, lena + 1);
    cmpinfo->cb_matresz (cmpinfo->userdata_matrix2, lenb + 1, lena + 1);

    for (i = 0; i <= lena; i ++) {
        /*g_matrix_val[0][i] = i; */
        /*g_matrix_dir[0][i] = EDIS_INSERT; */
        cmpinfo->cb_matset (cmpinfo->userdata_matrix,  0, i, i);
        cmpinfo->cb_matset (cmpinfo->userdata_matrix2, 0, i, EDIS_DELETE);
    }
    for (i = 0; i <= lenb; i ++) {
        //g_matrix_val[i* (lena + 1)] = i;
        //g_matrix_dir[i* (lena + 1)] = EDIS_INSERT;
        cmpinfo->cb_matset (cmpinfo->userdata_matrix,  i, 0, i);
        cmpinfo->cb_matset (cmpinfo->userdata_matrix2, i, 0, EDIS_INSERT);
    }
    /*g_matrix_dir[0][0] = EDIS_NONE; */
    cmpinfo->cb_matset (cmpinfo->userdata_matrix2, 0, 0, EDIS_NONE);
    for (i = 1; i <= lenb; i ++) {
        for (j = 1; j <= lena; j ++) {
            /*pi = g_matrix_val[i - 1][j] + 1; */
            /*pd = g_matrix_val[i][j - 1] + 1; */
            /*pr = g_matrix_val[i - 1][j - 1] + (strb[i - 1] == stra[j - 1]?0:1); */
            pi = 1 + cmpinfo->cb_matget (cmpinfo->userdata_matrix,  i - 1, j);
            pd = 1 + cmpinfo->cb_matget (cmpinfo->userdata_matrix,  i, j - 1);
            flg_equ = (0 == cmpinfo->cb_comp(cmpinfo->userdata_str, j - 1, i - 1));
            pr = cmpinfo->cb_matget (cmpinfo->userdata_matrix,  i - 1, j - 1) + (flg_equ?0:1);
            if (pi < pd) {
                if (pr < pi) {
                    /* pr < pi < pd */
                    //g_matrix_val[i * (lena + 1) + j] = pr;
                    cmpinfo->cb_matset (cmpinfo->userdata_matrix, i, j, pr);
                    if (flg_equ) {
                        //g_matrix_dir[i * (lena + 1) + j] = EDIS_IGNORE;
                        cmpinfo->cb_matset (cmpinfo->userdata_matrix2, i, j, EDIS_IGNORE);
                    } else {
                        //g_matrix_dir[i * (lena + 1) + j] = EDIS_REPLAC;
                        cmpinfo->cb_matset (cmpinfo->userdata_matrix2, i, j, EDIS_REPLAC);
                    }
                } else { /* pr >= pi */
                    //g_matrix_val[i][j] = pi;
                    //g_matrix_dir[i][j] = EDIS_INSERT;
                    cmpinfo->cb_matset (cmpinfo->userdata_matrix, i, j, pi);
                    cmpinfo->cb_matset (cmpinfo->userdata_matrix2, i, j, EDIS_INSERT);
                }
            } else { /* pi >= pd */
                if (pr < pi) {
                    if (pd < pr) {
                        /* pd < pr < pi */
                        //g_matrix_val[i * (lena + 1) + j] = pd;
                        //g_matrix_dir[i * (lena + 1) + j] = EDIS_DELETE;
                        cmpinfo->cb_matset (cmpinfo->userdata_matrix, i, j, pd);
                        cmpinfo->cb_matset (cmpinfo->userdata_matrix2, i, j, EDIS_DELETE);
                    } else { /* pd >= pr */
                        /* pr <= pd <= pi */
                        //g_matrix_val[i * (lena + 1) + j] = pr;
                        cmpinfo->cb_matset (cmpinfo->userdata_matrix, i, j, pr);
                        if (flg_equ) {
                            //g_matrix_dir[i * (lena + 1) + j] = EDIS_IGNORE;
                            cmpinfo->cb_matset (cmpinfo->userdata_matrix2, i, j, EDIS_IGNORE);
                        } else {
                            //g_matrix_dir[i * (lena + 1) + j] = EDIS_REPLAC;
                            cmpinfo->cb_matset (cmpinfo->userdata_matrix2, i, j, EDIS_REPLAC);
                        }
                    }
                } else { /* pi <= pr */
                    /* pd <= pi <= pr */
                    //g_matrix_val[i * (lena + 1) + j] = pd;
                    //g_matrix_dir[i * (lena + 1) + j] = EDIS_DELETE;
                    cmpinfo->cb_matset (cmpinfo->userdata_matrix, i, j, pd);
                    cmpinfo->cb_matset (cmpinfo->userdata_matrix2, i, j, EDIS_DELETE);
                }
            }
        }
    }
#if USE_OUT_ED_TABLE
    TRACE ("----|----|");
    for (i = 0; i < lena; i ++) {
        TRACE (" ");
        cmpinfo->cb_output (cmpinfo->userdata_str, stderr, 0, i);
        TRACE (" |");
    }
    TRACE ("\n");
    TRACE ("----|");
    for (i = 0; i <= lena; i ++) {
        TRACE (" %2d |", cmpinfo->cb_matget (cmpinfo->userdata_matrix, 0, i));
    }
    TRACE ("\n");
    TRACE ("----|");
    i = 0;
    for (j = 0; j <= cmpinfo->cb_len(cmpinfo->userdata_str, 0); j ++) {
        TRACE ("^ ");
        TRACE ("%s", edaction_val2cstr (cmpinfo->cb_matget (cmpinfo->userdata_matrix2, i, j)));
        TRACE ("^|");
    }
    TRACE ("\n");
    for (i = 1; i <= lenb; i ++) {
        TRACE (" ");
        cmpinfo->cb_output (cmpinfo->userdata_str, stderr, 1, i-1);
        TRACE (" |");
        //TRACE ("% 3d|", i);
        j = 0; TRACE (" %2d |", cmpinfo->cb_matget (cmpinfo->userdata_matrix, i, j));
        for (j = 1; j <= lena; j ++) {
            //g_matrix_dir[i * (lena + 1) + j]
            TRACE (" %2d |", cmpinfo->cb_matget (cmpinfo->userdata_matrix, i, j));
        }
        TRACE ("\n");
        TRACE ("----|");
        for (j = 0; j <= cmpinfo->cb_len(cmpinfo->userdata_str, 0); j ++) {
            TRACE ("^ ");
            TRACE ("%s", edaction_val2cstr (cmpinfo->cb_matget (cmpinfo->userdata_matrix2, i, j)));
            TRACE ("^|");
        }
        TRACE ("\n");
    }
#endif
    i = lenb;
    j = lena;
    assert (i > 0);
    assert (j > 0);
    for (pi = lena + lenb; pi > 0; pi --) {
        if (i < 0 || j < 0) {
            DBGMSG (PFDBG_CATLOG_PF, PFDBG_LEVEL_ERROR, "Implementation bug!!");
            DBGMSG (PFDBG_CATLOG_PF, PFDBG_LEVEL_ERROR, "The i,j overflow: lena(%d)+lenb(%d)=(%d), pi(%d), pi(%d), pi(%d)", lena, lenb, lena + lenb, pi, i, j);
            assert (pi >= 0);
            assert (i >= 0);
            assert (j >= 0);
            assert (0);
            break;
        }
        if (EDIS_NONE == cmpinfo->cb_matget (cmpinfo->userdata_matrix2, i, j) ) {
            break;
        }
        path[pi - 1] = cmpinfo->cb_matget (cmpinfo->userdata_matrix2, i, j);
#if USE_OUT_ED_TABLE
        TRACE ("path[%d]: %s, (i,j) from (%d,%d)", pi, edaction_val2cstr(path[pi - 1]), i, j);
#endif
        switch (path[pi - 1]) {
        case EDIS_INSERT:
            i --;
            break;
        case EDIS_DELETE:
            j --;
            break;
        case EDIS_REPLAC:
        case EDIS_IGNORE:
            i --;
            j --;
            break;
        case EDIS_NONE:
        default:
            assert (0);
            break;
        }
#if USE_OUT_ED_TABLE
        TRACE ("to (%d,%d)\n", i, j);
#endif
    }
    if (pi > 0) {
        memmove (path, &(path[pi]), sizeof (char) * (lena + lenb - pi));
    }
    *ret_numpath = lena + lenb - pi;
#if USE_OUT_ED_TABLE
    assert (NULL != path);
    //printf ("convert from string 1: %s\nTO %s\n", stra, strb);
    TRACE ("path: ");
    for (i = 0; i < *ret_numpath; i ++) {
        TRACE ("%s", edaction_val2cstr (path[i]));
        TRACE (" ");
    }
    TRACE ("\n");
#endif
    return cmpinfo->cb_matget (cmpinfo->userdata_matrix, lenb, lena);
}

/**********************************************************************************/
typedef struct _mymatrix_t {
    int *buf;
    size_t szbuf; // the # of elements in the matrix
    size_t szcol; // number of columns
} mymatrix_t;

int
mymat_init (void *userdata)
{
    mymatrix_t *pm = (mymatrix_t *) userdata;
    memset (pm, 0, sizeof (*pm));
    return 0;
}

int
mymat_clear (void *userdata)
{
    mymatrix_t *pm = (mymatrix_t *) userdata;
    if (NULL != pm->buf) {
        free (pm->buf);
    }
    memset (pm, 0, sizeof (*pm));
    return 0;
}

/* resize the matrix to (row, col), and clear it */
int
mymat_resize (void *userdata, size_t row, size_t col)
{
    int * newbuf = NULL;
    mymatrix_t *pm = (mymatrix_t *) userdata;
    size_t newsize = row * col;

    if (NULL == pm->buf) {
        pm->szbuf = 0;
        pm->szcol = 0;
    }
    if (newsize <= pm->szbuf) {
        memset (pm->buf, 0, sizeof(int) * newsize);
        pm->szcol = col;
        return 0;
    }
    if (NULL == pm->buf) {
        newbuf = (int *)malloc (newsize * sizeof(int));
    } else {
        assert (newsize > pm->szbuf);
        newbuf = (int *)realloc (pm->buf, sizeof(int) * newsize);
    }
    if (NULL == newbuf) {
        return -1;
    }
    pm->buf = newbuf;
    pm->szbuf = newsize;
    pm->szcol = col;
    memset (pm->buf, 0, sizeof(int) * newsize);
    return 0;
}

/* get the value at (row, col) */
int
mymat_get (void *userdata, size_t row, size_t col)
{
    mymatrix_t *pm = (mymatrix_t *) userdata;
    assert (NULL != pm);
    assert (row * pm->szcol + col < pm->szbuf);
    assert (NULL != pm->buf);
    return pm->buf[row * pm->szcol + col];
}

/* set the value at (row, col) */
int
mymat_set (void *userdata, size_t row, size_t col, int val)
{
    mymatrix_t *pm = (mymatrix_t *) userdata;
    assert (NULL != pm);
#if 1 // DEBUG
	if (row * pm->szcol + col >= pm->szbuf) {
		fprintf (stderr, "Error: row=%d,col=%d,szcol=%d,szbuf=%d\n", (int)row,(int)col,(int)pm->szcol,(int)pm->szbuf);
	}
#endif
    assert (row * pm->szcol + col < pm->szbuf);
    assert (NULL != pm->buf);
    pm->buf[row * pm->szcol + col] = val;
    return 0;
}

/**********************************************************************************/
#if DEBUG
typedef struct _twocstr_t {
    char *str[2];
} twocstr_t;

/* idx -- the index of the string; right -- 0 -- `left' string, 1 -- `right' string */
int
strcmp_output_cstr (void *userdata, FILE *fp, int right, size_t idx)
{
    twocstr_t * ptcs = (twocstr_t *)userdata;
    assert (NULL != userdata);
    return fprintf (fp, "%c", ptcs->str[right % 2][idx]);
}

/* idx1 -- the index of the `left' string; idx2 -- right */
int
strcmp_comp_cstr (void *userdata, size_t idx1, size_t idx2)
{
    twocstr_t * ptcs = (twocstr_t *)userdata;
    assert (NULL != userdata);
    if (ptcs->str[0][idx1] == ptcs->str[1][idx2]) {
        return 0;
    }
    if (ptcs->str[0][idx1] < ptcs->str[1][idx2]) {
        return -1;
    }
    return 1;
}

/* 0 -- `left' string, 1 -- `right' string */
int
strcmp_length_cstr (void *userdata, int right)
{
    twocstr_t * ptcs = (twocstr_t *)userdata;
    assert (NULL != userdata);
    return strlen (ptcs->str[right % 2]);
}

/* idx -- the index of the string; right -- 0 -- `left' string, 1 -- `right' string */
wchar_t
strcmp_cb_getval_cstr (void *userdata, int right, size_t idx)
{
    twocstr_t * ptcs = (twocstr_t *)userdata;
    return (wchar_t) ptcs->str[right % 2][idx];
}

void test1()
{
#define STR1 "ALGORITHM"
#define STR2 "ALTRUISTIC"
//#define STR1 "aboooout"
//#define STR2 "about"
//#define STR1 "TAGGTCA-"
//#define STR2 "AACAGTTACC-"
    int ret;

    twocstr_t tstr;

    mymatrix_t mat1;
    mymatrix_t mat2;
    mymat_init (&mat1);
    mymat_init (&mat2);

    tstr.str[0] = (char *)STR1;
    tstr.str[1] = (char *)STR2;

    strcmp_t cmpinfo;
    cmpinfo.userdata_str = &tstr;
    cmpinfo.cb_comp = strcmp_comp_cstr;
    cmpinfo.cb_len  = strcmp_length_cstr;
    cmpinfo.cb_getval  = strcmp_cb_getval_cstr;
    cmpinfo.userdata_matrix  = &mat1;
    cmpinfo.userdata_matrix2 = &mat2;
    cmpinfo.cb_matget  = mymat_get;
    cmpinfo.cb_matset  = mymat_set;
    cmpinfo.cb_matresz = mymat_resize;
    cmpinfo.cb_output = strcmp_output_cstr;

    char path[sizeof(STR1) + sizeof(STR2) + 20];
    size_t szpath = sizeof(path)-1;

    fprintf (stderr, "str1: %s\n", STR1);
    fprintf (stderr, "str2: %s\n", STR2);
    ret = ed_edit_distance (&cmpinfo);
    fprintf (stderr, "different sites 1 = %d\n", ret);
    ret = ed_edit_distance_path (&cmpinfo, path, &szpath);
    fprintf (stderr, "different sites 2 = %d\n", ret);

    mymat_clear (&mat1);
    mymat_clear (&mat2);
}
#endif // DEBUG


/**********************************************************************************/
typedef struct _wcstrpair_t {
    wchar_t * str[2]; /* the file contents are transfered to a wchar_t buffer (some contents were filtered out according to user commanded) */
    size_t szstr[2];    /* the max number of the items of str[] */
    size_t len[2];    /* the number of the char in the str[] */
    size_t * pos[2];   /* the start position of the real data */
} wcstrpair_t;

int
wcspair_init (wcstrpair_t *wp)
{
    memset (wp, 0, sizeof(*wp));
    return 0;
}

int
wcspair_clear (wcstrpair_t *wp)
{
    if (NULL != wp->str[0]) {
        free (wp->str[0]);
    }
    if (NULL != wp->str[1]) {
        free (wp->str[1]);
    }
    if (NULL != wp->pos[0]) {
        free (wp->str[0]);
    }
    if (NULL != wp->pos[1]) {
        free (wp->str[1]);
    }
    return 0;
}


/* idx -- the index of the string; right -- 0 -- `left' string, 1 -- `right' string */
int
strcmp_output_utf8fp (void *userdata, FILE *fp, int right, size_t idx)
{
    uint8_t buffer[10];
    wcstrpair_t * ptcs = (wcstrpair_t *)userdata;
    assert (NULL != userdata);
    memset (buffer, 0, sizeof(buffer));
    switch (ptcs->str[right % 2][idx]) {
    case '\r':
        sprintf ((char *)buffer, "\\r");
        break;
    case '\n':
        sprintf ((char *)buffer, "\\n");
        break;
    default:
        uni_to_utf8 (ptcs->str[right % 2][idx], buffer, sizeof(buffer) - 1);
        break;
    }
    return fprintf (fp, "%2s", buffer);
}

/* idx1 -- the index of the `left' string; idx2 -- right */
int
strcmp_comp_utf8fp (void *userdata, size_t idx1, size_t idx2)
{
    wcstrpair_t * ptcs = (wcstrpair_t *)userdata;
    assert (NULL != userdata);
    if (ptcs->str[0][idx1] == ptcs->str[1][idx2]) {
        return 0;
    }
    if (ptcs->str[0][idx1] < ptcs->str[1][idx2]) {
        return -1;
    }
    return 1;
}

/* 0 -- `left' string, 1 -- `right' string */
int
strcmp_length_utf8fp (void *userdata, int right)
{
    wcstrpair_t * ptcs = (wcstrpair_t *)userdata;
    assert (NULL != userdata);
    return (ptcs->len[right % 2]);
}

/* idx -- the index of the string; right -- 0 -- `left' string, 1 -- `right' string */
wchar_t
strcmp_cb_getval_utf8fp (void *userdata, int right, size_t idx)
{
    wcstrpair_t * ptcs = (wcstrpair_t *)userdata;
    return ptcs->str[right % 2][idx];
}

int
strcmp_cb_addval_utf8fp (void *userdata, int right, wchar_t val)
{
    wcstrpair_t *wp = (wcstrpair_t *)userdata;
    // check the buffer size:
    if (wp->szstr[right % 2] <= wp->len[right % 2]) {
        size_t newsize = wp->szstr[right % 2] * 2;
        if (newsize < 200) {
            newsize = 200;
        }
        wchar_t * newbuf = (wchar_t *)realloc (wp->str[right % 2], sizeof(wchar_t) * newsize);
        if (NULL == newbuf) {
            return -1;
        }
        wp->szstr[right % 2] = newsize;
        wp->str[right % 2] = newbuf;
    }
    wp->str[right % 2][wp->len[right % 2]] = val;
    wp->len[right % 2] ++;
    return 0;
}

/**********************************************************************************/
// off: the offset of the first char in the file
// buf: the line buffer
int
process (wcstrpair_t *wp, int right, off_t off, uint8_t *buf) //, size_t szbuf)
{
    wchar_t wch = 0;
    uint8_t * p = buf;
    uint8_t * pnext = NULL;
    while ((pnext = get_utf8_value (p, &wch)) != NULL) {
        if (wch == 0) {
            break;
        }

        if (0xFEFF == wch) {
            fprintf (stderr, "BOM detected!\n");
        } else {
            //write (1, &wch, sizeof (wch));
            // latex comments:
            //if ('%' == wch) {
                //break;
            //}
            if (0 != strcmp_cb_addval_utf8fp (wp, right, wch)) {
                return -1;
            }
        }
        p = pnext;
    }
    //strcmp_cb_addval_utf8fp (wp, right, '\n');
    return 0;
}

int
load_file (wcstrpair_t *wp, int right, FILE *fp)
{
    char * buffer = NULL;
    size_t szbuf = 0;
    off_t pos;

    szbuf = 10000;
    buffer = (char *) malloc (szbuf);
    if (NULL == buffer) {
        return -1;
    }
    pos = ftell (fp);
    //ssize_t getline (char **lineptr, size_t *n, FILE *stream)
    //while ( fgets ( (char *)buffer, sizeof buffer, fp ) != NULL ) {
    while ( getline ( &buffer, &szbuf, fp ) > 0 ) {
        process (wp, right, pos, (uint8_t *)buffer);
        pos = ftell (fp);
    }

    free (buffer);
    return 0;
}

void generate_compare_file(wcstrpair_t *wp)
{
    int i;
    int ret;

    mymatrix_t mat1;
    mymatrix_t mat2;
    mymat_init (&mat1);
    mymat_init (&mat2);

    strcmp_t cmpinfo;
    cmpinfo.userdata_str = wp;
    cmpinfo.cb_comp = strcmp_comp_utf8fp;
    cmpinfo.cb_len  = strcmp_length_utf8fp;
    cmpinfo.cb_getval = strcmp_cb_getval_utf8fp;
    cmpinfo.userdata_matrix  = &mat1;
    cmpinfo.userdata_matrix2 = &mat2;
    cmpinfo.cb_matget  = mymat_get;
    cmpinfo.cb_matset  = mymat_set;
    cmpinfo.cb_matresz = mymat_resize;
    cmpinfo.cb_output = strcmp_output_utf8fp;

    size_t szpath = strcmp_length_utf8fp(wp, 0) + strcmp_length_utf8fp(wp,1) + 200;
    char * path = NULL;
    path = (char *)malloc (szpath + 1);
    if (NULL == path) {
        perror ("malloc");
        return;
    }

#if USE_OUT_ED_TABLE
    ret = ed_edit_distance (&cmpinfo);
    fprintf (stderr, "different sites 1 = %d\n", ret);
#endif
    ret = ed_edit_distance_path (&cmpinfo, path, &szpath);
    fprintf (stderr, "different sites = %d\n", ret);

    int x = 0; // index of string 1
    int y = 0; // index of string 2
    wchar_t val = 0;
    for (i = 0; (size_t)i < szpath; i ++) {
        val = 0;
        switch (path[i]) {
        case EDIS_NONE:
            //printf ("0");
            break;
        case EDIS_INSERT:
            val = cmpinfo.cb_getval (cmpinfo.userdata_str, 1, y);
            printf ("<ins>");
            cmpinfo.cb_output (cmpinfo.userdata_str, stdout, 1, y);
            printf ("</ins>");
            y ++;
            break;
        case EDIS_DELETE:
            val = cmpinfo.cb_getval (cmpinfo.userdata_str, 0, x);
            printf ("<del>");
            cmpinfo.cb_output (cmpinfo.userdata_str, stdout, 0, x);
            printf ("</del>");
            x ++;
            break;
        case EDIS_REPLAC:
            val = cmpinfo.cb_getval (cmpinfo.userdata_str, 1, y);
            printf ("<del>");
            cmpinfo.cb_output (cmpinfo.userdata_str, stdout, 0, x);
            printf ("</del>");
            x ++;
            printf ("<ins>");
            cmpinfo.cb_output (cmpinfo.userdata_str, stdout, 1, y);
            printf ("</ins>");
            y ++;
            break;
        case EDIS_IGNORE:
            val = cmpinfo.cb_getval (cmpinfo.userdata_str, 0, x);
            cmpinfo.cb_output (cmpinfo.userdata_str, stdout, 0, x);
            x ++;
            y ++;
            break;
        }
        if ('\n' == val) {
            printf ("<br />");
        }
    }

    mymat_clear (&mat1);
    mymat_clear (&mat2);
    free (path);
}

#define HTML_OUT_HEADER \
    "<!DOCTYPE html>" "\n" \
    "<html>" "\n" \
    "<head>" "\n" \
    "<meta charset='utf-8' />" "\n" \
    "<style>" "\n" \
    "  ins {" "\n" \
    "    color:purple;" "\n" \
    "  }" "\n" \
    "  del," "\n" \
    "  strike {" "\n" \
    "    color:grey;" "\n" \
    "    text-decoration: none;" "\n" \
    "    line-height: 1.4;" "\n" \
    "    background-image: -webkit-gradient(linear, left top, left bottom, from(transparent), color-stop(0.63em, transparent), color-stop(0.63em, #ff0000), color-stop(0.7em, #ff0000), color-stop(0.7em, transparent), to(transparent));" "\n" \
    "    background-image: -webkit-linear-gradient(top, transparent 0em, transparent 0.63em, #ff0000 0.63em, #ff0000 0.7em, transparent 0.7em, transparent 1.4em);" "\n" \
    "    background-image: -o-linear-gradient(top, transparent 0em, transparent 0.63em, #ff0000 0.63em, #ff0000 0.7em, transparent 0.7em, transparent 1.4em);" "\n" \
    "    background-image: linear-gradient(to bottom, transparent 0em, transparent 0.63em, #ff0000 0.63em, #ff0000 0.7em, transparent 0.7em, transparent 1.4em);" "\n" \
    "    -webkit-background-size: 1.4em 1.4em;" "\n" \
    "    background-size: 1.4em 1.4em;" "\n" \
    "    background-repeat: repeat;" "\n" \
    "  }" "\n" \
    "</style>" "\n" \
    "<title>compcoll Generated compare text</title>" "\n" \
    "</head>" "\n" \
    "<body>"


#define HTML_OUT_TAIL \
    "</body>" "\n" \
    "</html>"

char flg_nohtmlhdr = 0;

int
compare_files (char * filename1, char *filename2)
{
    FILE *fp1 = NULL;
    FILE *fp2 = NULL;
    wcstrpair_t wpinfo;

    fp1 = fopen (filename1, "r");
    if (NULL == fp1) {
        perror ( filename1 );
        fprintf (stderr, "Not found file: %s\n", filename1);
        goto end_compfile;
    }

    fp2 = fopen (filename2, "r");
    if (NULL == fp2) {
        perror ( filename2 );
        fprintf (stderr, "Not found file: %s\n", filename2);
        goto end_compfile;
    }

    wcspair_init (&wpinfo);
    load_file (&wpinfo, 0, fp1);
    load_file (&wpinfo, 1, fp2);

    if (! flg_nohtmlhdr) {
        printf ("%s\n", HTML_OUT_HEADER);
    }
    printf ("<h3>Compared files:</h3>\n");
    printf ("<table>");
    printf ("<tr><td>original file:</td><td><b>%s</b><td/></tr>\n", filename1);
    printf ("<tr><td>new file:</td>     <td><b>%s</b><td/></tr>\n", filename2);
    printf ("</table>\n");
    generate_compare_file(&wpinfo);
    if (! flg_nohtmlhdr) {
        printf ("\n%s\n", HTML_OUT_TAIL);
    }

    wcspair_clear (&wpinfo);

end_compfile:
    if (fp1) fclose (fp1);
    if (fp2) fclose (fp2);
    return 0;
}

int
main (int argc, char * argv[])
{
    int c;
    struct option longopts[]  = {
        //{ "port",         1, 0, 'p' },
        //{ "recursive",    0, 0, 'r' },
        { "htmlheader",   0, 0, 'H' },
        { "htmltail",     0, 0, 'T' },
        { "htmlcontent",  0, 0, 'C' },

        { "help",         0, 0, 'h' },
        { "verbose",      0, 0, 'v' },
        { 0,              0, 0,  0  },
    };

    while ((c = getopt_long( argc, argv, "HTCvh", longopts, NULL )) != EOF) {
        switch (c) {
        case 'H':
            printf ("%s\n", HTML_OUT_HEADER);
            exit (0);
            break;
        case 'T':
            printf ("\n%s\n", HTML_OUT_TAIL);
            exit (0);
            break;
        case 'C':
            flg_nohtmlhdr = 1;
            break;

        case 'v':
            break;

        case 'h':
            usage (argv[0]);
            exit (0);
            break;
        default:
            fprintf (stderr, "Unknown parameter: '%c'.\n", c);
            fprintf (stderr, "Use '%s -h' for more information.\n", basename(argv[0]));
            exit (-1);
            break;
        }
    }

    //test1(); return 0;

    c = optind;
    //for (; c < (size_t)argc; c ++) {
    compare_files (argv[c], argv[c + 1]);
    return 0;
}
