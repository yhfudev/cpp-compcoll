/**
 * @file    mymat.c
 * @brief   a simple matrix memory
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @license GPL 2.0/LGPL 2.1
 * @date    2016-03-13
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mymat.h"

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

#include "editdistance.h"

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
