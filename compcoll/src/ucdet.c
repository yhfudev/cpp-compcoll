/**
 * @file    ucdet.c
 * @brief   Universal charset detection use the library by Mozilla.
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2014-09-07
 */

#include <stdint.h>    /* uint8_t */
#include <stdlib.h>    /* size_t */
#include <unistd.h> // write()
#include <sys/types.h> /* ssize_t */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include "getline.h"
#include "i18n.h"

/**********************************************************************************/
#if 0
#define VER_MAJOR 0
#define VER_MINOR 1
#define VER_MOD   1

static void
version (void)
{
    fprintf (stderr, "Universal charset detection use the library by Mozilla.\n");
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
#endif

/**********************************************************************************/

int
load_file (chardet_t det, FILE *fp)
{
    char * buffer = NULL;
    size_t szbuf = 0;
    //off_t pos;

    szbuf = 10000;
    buffer = (char *) malloc (szbuf);
    if (NULL == buffer) {
        return -1;
    }
    //pos = ftell (fp);
    while ( getline ( &buffer, &szbuf, fp ) > 0 ) {
        chardet_handle_data (det, (const char*)buffer, (unsigned int)strlen ((char *)buffer));
        //pos = ftell (fp);
    }

    free (buffer);
    return 0;
}

int
load_filename (chardet_t det, char * filename)
{
    FILE *fp = NULL;
    fp = fopen (filename, "r");

    load_file (det, fp);

    fclose (fp);
    return 0;
}

int
main (int argc, char * argv[])
{
    char encname[CHARDET_MAX_ENCODING_NAME];
    chardet_t det = NULL;

    chardet_create (&det);

    if (argc > 1) {
        load_filename (det, argv[1]);
    } else {
        load_file (det, stdin);
    }

    chardet_data_end (det);
    if (chardet_get_charset (det, encname, CHARDET_MAX_ENCODING_NAME) < 0) {
        DBGMSG (PFDBG_CATLOG_USR_PLUGIN, PFDBG_LEVEL_WARNING, "Error in detect charset encoding!\n");
    } else {
        DBGMSG (PFDBG_CATLOG_USR_PLUGIN, PFDBG_LEVEL_INFO, "1 detected charset encoding: '%s'!\n", encname);
    }
    chardet_destroy (det);
    fprintf (stdout, "%s\n", encname);
    return 0;
}
