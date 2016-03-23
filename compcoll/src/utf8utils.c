/**
 * @file    utf8utils.c
 * @brief   utf8 functions
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @license GPL 2.0/LGPL 2.1
 * @date    2016-03-07
 */

#include <assert.h>

#include "utf8utils.h"

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

