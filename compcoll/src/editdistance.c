/**
 * @file    editdistance.c
 * @brief   Calculate the Edit Distance between two strings, includes both complete and optimized versions
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @license GPL 2.0/LGPL 2.1
 * @date    2016-03-13
 */

#include <string.h>
#include <assert.h>

#include "editdistance.h"

#define MIN(a,b) (((a)<(b))?(a):(b))

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
