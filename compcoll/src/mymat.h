/**
 * @file    mymat.h
 * @brief   a simple matrix memory
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @license GPL 2.0/LGPL 2.1
 * @date    2016-03-13
 */

#ifndef __MY_EDCSTR_H
#define __MY_EDCSTR_H

#include <stdlib.h>    /* size_t */

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

typedef struct _mymatrix_t {
    int *buf;
    size_t szbuf; // the # of elements in the matrix
    size_t szcol; // number of columns
} mymatrix_t;

int mymat_init (void *userdata);
int mymat_clear (void *userdata);
int mymat_resize (void *userdata, size_t row, size_t col);
int mymat_get (void *userdata, size_t row, size_t col);
int mymat_set (void *userdata, size_t row, size_t col, int val);

#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /* __MY_EDCSTR_H */
