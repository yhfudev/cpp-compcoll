/**
 * @file    editdistance.h
 * @brief   Calculate the Edit Distance between two strings, includes both complete and optimized versions
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @license GPL 2.0/LGPL 2.1
 * @date    2016-03-13
 */

#ifndef __MY_EDITDISTANCE_H
#define __MY_EDITDISTANCE_H

#define _GNU_SOURCE 1
#include <stdint.h>    /* uint8_t */
#include <stdio.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

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

#define EDIS_NONE   0x00 /*!< No action */
#define EDIS_INSERT 0x01 /*!< Insert action */
#define EDIS_DELETE 0x02 /*!< Delete action */
#define EDIS_REPLAC 0x03 /*!< Replace action */
#define EDIS_IGNORE 0x04 /*!< Ignore action */

const char * edaction_val2cstr (char val);
int ed_edit_distance (strcmp_t *cmpinfo);
int ed_edit_distance_path (strcmp_t *cmpinfo, char *path, size_t *ret_numpath);

#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /* __MY_EDITDISTANCE_H */
