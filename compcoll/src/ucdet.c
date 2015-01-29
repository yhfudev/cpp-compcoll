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
        "\t%s [files ...]\n"
        , basename(progname));
    fprintf (stderr, "\nOptions:\n");
    fprintf (stderr, "\tfiles...\tThe list of files group, if none, read from STDIN.\n");
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

int
load_file (chardet_t *det, FILE *fp)
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
        chardet_parse (det, (const char*)buffer, (unsigned int)strlen ((char *)buffer));
        //pos = ftell (fp);
    }

    free (buffer);
    return 0;
}

int
load_filename (chardet_t * det, char * filename)
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

    int c;
    struct option longopts[]  = {
        { "help",         0, 0, 'h' },
        { "verbose",      0, 0, 'v' },
        { 0,              0, 0,  0  },
    };

    while ((c = getopt_long( argc, argv, "vh", longopts, NULL )) != EOF) {
        switch (c) {
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

    chardet_init (&det);

    c = optind;
    if (argc > 1) {
        int i;
        for (i = c; i < argc; i ++) {
            load_filename (&det, argv[i]);
        }
    } else {
        load_file (&det, stdin);
    }

    chardet_end (&det);
    if (chardet_results (&det, encname, CHARDET_MAX_ENCODING_NAME) < 0) {
        DBGMSG (PFDBG_CATLOG_USR_PLUGIN, PFDBG_LEVEL_WARNING, "Error in detect charset encoding!\n");
    } else {
        DBGMSG (PFDBG_CATLOG_USR_PLUGIN, PFDBG_LEVEL_INFO, "1 detected charset encoding: '%s'!\n", encname);
    }
    chardet_clear (&det);
    fprintf (stdout, "%s\n", encname);
    return 0;
}
