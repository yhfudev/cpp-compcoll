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
#include "utf8utils.h"
#include "editdistance.h"
#include "mymat.h"

#if _WIN32
#define tmpfile64() tmpfile()
//#define ftruncate(a,b,c) _chsize((a),(b),(c))
#endif

/**********************************************************************************/

#define VER_MAJOR 0
#define VER_MINOR 1
#define VER_MOD   2

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
        "\t%s [options] <old file> <new file>\n"
        , basename(progname));
    fprintf (stderr, "\nOptions:\n");
    //fprintf (stderr, "\t-p <port #>\tthe listen port\n");
    fprintf (stderr, "\t-H\tshow the HTML header\n");
    fprintf (stderr, "\t-T\tshow the HTML tail\n");
    fprintf (stderr, "\t-C\tshow the HTML content only(no HTML header and tail)\n");
    fprintf (stderr, "\t-m\tmerge the same changes\n");
    fprintf (stderr, "\t-r\toutput <return>, all|old|new\n");
    fprintf (stderr, "\t-x\tset the sequence # of the title\n");
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

// how to output the <return> char?
#define OUT_RET_OLD 0x01 /* output the <return> according the old file */
#define OUT_RET_NEW 0x02 /* output the <return> according the new file */

typedef struct _wcstrpair_t {
    wchar_t * str[2];   /* the file contents are transfered to a wchar_t buffer (some contents were filtered out according to user commanded) */
    size_t szstr[2];    /* the max number of the items of str[] */
    size_t len[2];      /* the number of the char in the str[] */
    size_t * pos[2];    /* the start position of the real data */
    FILE * tmpfp[2];    /* the temp file for merge same changes */
    char flg_outret;    /* how to output the <return> char? OUT_RET_(OLD|NEW) */
} wcstrpair_t;

// flg_merge: 1  - merge the same <del>/<ins>
int
wcspair_init (wcstrpair_t *wp, char flg_merge)
{
    memset (wp, 0, sizeof(*wp));
    wp->flg_outret = OUT_RET_NEW;
    if (flg_merge) {
        wp->tmpfp[0] = tmpfile64();
        wp->tmpfp[1] = tmpfile64();
        if ((NULL == wp->tmpfp[0]) || (NULL == wp->tmpfp[1])) {
            perror ("tmpfile64");
            if (NULL != wp->tmpfp[0]) {
                fclose (wp->tmpfp[0]);
                wp->tmpfp[0] = NULL;
            }
            if (NULL != wp->tmpfp[1]) {
                fclose (wp->tmpfp[1]);
                wp->tmpfp[1] = NULL;
            }
        }
    }
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
    if (NULL != wp->tmpfp[0]) {
        fclose (wp->tmpfp[0]);
        wp->tmpfp[0] = NULL;
    }
    if (NULL != wp->tmpfp[1]) {
        fclose (wp->tmpfp[1]);
        wp->tmpfp[1] = NULL;
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

off_t
fp_size (FILE *fp)
{
    off_t pos = ftell(fp);
    off_t sz = 0;
    fseek(fp, 0L, SEEK_END);
    sz = ftell(fp);
    fseek(fp, pos, SEEK_SET);
    return sz;
}

#define TMPFPIDX_DEL 0
#define TMPFPIDX_INS 1

// output the strings in the buffer
void
wpair_output_flush (wcstrpair_t *wp)
{
    char flg_err = 0;
    char a;
    if (NULL == wp->tmpfp[0]) {
        return;
    }
    // copy file content
    if (fp_size (wp->tmpfp[TMPFPIDX_DEL]) > 0) {
        fprintf (stdout, "<del>");
        fseek(wp->tmpfp[TMPFPIDX_DEL], 0, SEEK_SET);
        while ((a = fgetc(wp->tmpfp[TMPFPIDX_DEL])) != EOF) {
            if (0 == a) continue;
            fputc (a, stdout);
        }
        fprintf (stdout, "</del>");
    }
    if (fp_size (wp->tmpfp[TMPFPIDX_INS]) > 0) {
        fprintf (stdout, "<ins>");
        fseek(wp->tmpfp[TMPFPIDX_INS], 0, SEEK_SET);
        while ((a = fgetc(wp->tmpfp[TMPFPIDX_INS])) != EOF) {
            if (0 == a) continue;
            fputc (a, stdout);
        }
        fprintf (stdout, "</ins>");
    }

    // truncate file
    if (ftruncate(fileno(wp->tmpfp[0]), 0) != 0) {
        perror ("truncate file0");
        flg_err = 1;
    }
    if (ftruncate(fileno(wp->tmpfp[1]), 0) != 0) {
        perror ("truncate file1");
        flg_err = 1;
    }
    if (flg_err) {
        fclose (wp->tmpfp[0]);
        fclose (wp->tmpfp[1]);
        wp->tmpfp[0] = NULL;
        wp->tmpfp[1] = NULL;
    }
}

// delete a char
void
wpair_output_del (wcstrpair_t *wp, strcmp_t *sp, int right, size_t idx)
{
    assert (NULL != wp);
    assert (NULL != sp);
    if (NULL == wp->tmpfp[0]) {
        fprintf (stdout, "<del>");
        sp->cb_output (sp->userdata_str, stdout, right, idx);
        fprintf (stdout, "</del>");
        return;
    }
    sp->cb_output (sp->userdata_str, wp->tmpfp[TMPFPIDX_DEL], right, idx);
}

// insert a new char
void
wpair_output_ins (wcstrpair_t *wp, strcmp_t *sp, int right, size_t idx)
{
    assert (NULL != wp);
    assert (NULL != sp);
    if (NULL == wp->tmpfp[0]) {
        fprintf (stdout, "<ins>");
        sp->cb_output (sp->userdata_str, stdout, right, idx);
        fprintf (stdout, "</ins>");
        return;
    }
    sp->cb_output (sp->userdata_str, wp->tmpfp[TMPFPIDX_INS], right, idx);
}

// the normal char
void
wpair_output_nor (wcstrpair_t *wp, strcmp_t *sp, int right, size_t idx)
{
    assert (NULL != wp);
    assert (NULL != sp);

    wpair_output_flush (wp);
    sp->cb_output (sp->userdata_str, stdout, right, idx);
}

// output a return
void
wpair_output_ret (wcstrpair_t *wp)
{
    assert (NULL != wp);

    wpair_output_flush (wp);
    fprintf (stdout, "<br />");
}

void
generate_compare_file(wcstrpair_t *wp)
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

    mymat_clear (&mat1);
    mymat_clear (&mat2);

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
            if (wp->flg_outret | OUT_RET_NEW) {
                val = cmpinfo.cb_getval (cmpinfo.userdata_str, 1, y);
            }
            wpair_output_ins (wp, &cmpinfo, 1, y);
            y ++;
            break;
        case EDIS_DELETE:
            if (wp->flg_outret | OUT_RET_OLD) {
                val = cmpinfo.cb_getval (cmpinfo.userdata_str, 0, x);
            }
            wpair_output_del (wp, &cmpinfo, 0, x);
            x ++;
            break;
        case EDIS_REPLAC:
            if (wp->flg_outret | OUT_RET_NEW) {
                val = cmpinfo.cb_getval (cmpinfo.userdata_str, 1, y);
            }
            if (wp->flg_outret | OUT_RET_OLD) {
                if ('\n' != val) {
                    val = cmpinfo.cb_getval (cmpinfo.userdata_str, 0, x);
                }
            }
            wpair_output_del (wp, &cmpinfo, 0, x);
            x ++;
            wpair_output_ins (wp, &cmpinfo, 1, y);
            y ++;
            break;
        case EDIS_IGNORE:
            val = cmpinfo.cb_getval (cmpinfo.userdata_str, 0, x);
            wpair_output_nor (wp, &cmpinfo, 0, x);
            x ++;
            y ++;
            break;
        }
        if ('\n' == val) {
            //fprintf (stdout, "<br />");
            wpair_output_ret (wp);
        }
    }

    free (path);
}

#define HTML_OUT_HEADER \
    "<!DOCTYPE html>" "\n" \
    "<html>" "\n" \
    "<head>" "\n" \
    "<meta charset='utf-8' />" "\n" \
    "<style>" "\n" \
    "  ins {" "\n" \
    "    color:blue; font-weight: normal;" "\n" \
    "  }" "\n" \
    "  del," "\n" \
    "  strike {" "\n" \
    "    color:purple;" "\n" \
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
    "<link href='compcollouter.css' rel='stylesheet' type='text/css'>" "\n" \
    "<title>compcoll Generated compare text</title>" "\n" \
    "</head>" "\n" \
    "<body>"


#define HTML_OUT_TAIL \
    "</body>" "\n" \
    "</html>"

char flg_nohtmlhdr = 0;


// flg_merge: 1  - merge the same <del>/<ins>
int
compare_files (ssize_t idx, char * filename1, char *filename2, char flg_merge, char flg_outret)
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

    wcspair_init (&wpinfo, flg_merge);
    wpinfo.flg_outret = flg_outret;
    load_file (&wpinfo, 0, fp1);
    load_file (&wpinfo, 1, fp2);

    if (! flg_nohtmlhdr) {
        printf ("%s\n", HTML_OUT_HEADER);
    }
    if (idx >= 0) {
        printf ("<h3>[%zd] Compared files:</h3>\n", idx);
    } else {
        printf ("<h3>Compared files:</h3>\n");
    }
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
    char flg_merge = 0;
    char flg_outret = OUT_RET_NEW;
    ssize_t idx = -1;
    int c;
    struct option longopts[]  = {
        { "htmlheader",   0, 0, 'H' },
        { "htmltail",     0, 0, 'T' },
        { "htmlcontent",  0, 0, 'C' },
        { "mergechanges", 0, 0, 'm' },
        { "outreturn",    1, 0, 'r' },
        { "indexnum",     1, 0, 'x' },

        { "help",         0, 0, 'h' },
        { "verbose",      0, 0, 'v' },
        { 0,              0, 0,  0  },
    };

    while ((c = getopt_long( argc, argv, "mr:x:HTCvh", longopts, NULL )) != EOF) {
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
        case 'r':
            if (0 == strcmp(optarg, "all")) {
                flg_outret = OUT_RET_NEW | OUT_RET_OLD;
            } else if (0 == strcmp(optarg, "old")) {
                flg_outret = OUT_RET_OLD;
            } else if (0 == strcmp(optarg, "new")) {
                flg_outret = OUT_RET_NEW;
            }
            break;
        case 'm':
            flg_merge = 1;
            break;
        case 'x':
            idx = atoi(optarg);
            break;

        case 'v':
            break;

        case 'h':
            usage (argv[0]);
            exit (0);
            break;
        default:
            fprintf (stderr, "%s: Unknown parameter: '%c'.\n", argv[0], c);
            fprintf (stderr, "Use '%s -h' for more information.\n", basename(argv[0]));
            exit (-1);
            break;
        }
    }

    //test1(); return 0;

    c = optind;
    //for (; c < (size_t)argc; c ++) {
    compare_files (idx, argv[c], argv[c + 1], flg_merge, flg_outret);
    return 0;
}
