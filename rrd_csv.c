/*****************************************************************************
 * RRDtool 1.GIT, Copyright by Tobi Oetiker
 *****************************************************************************
 * rrd_csv  Dumps RRD to CSV format
 *****************************************************************************
 * $Id$
 * $Log$
 * Revision 1.0  2004/05/25 20:53:21  fbsanchez
 * First version of this component
 *
 *****************************************************************************/
#include "rrd_tool.h"
#include "rrd_rpncalc.h"
#include "rrd_client.h"
#include "rrd_snprintf.h"
#include "rrd.h"


#if !(defined(NETWARE) || defined(WIN32))
extern char *tzname[2];
#endif

void PrintUsage() {
    printf("* dump an RRD to CSV\nUsage:\n"
       "\trrd2csv [--csv|-c]\n"
       "\t\t[--cf|-f avg|max|min|last]\n"
       "\t\tfile.rrd [file.xml]\n\n");
}

int RRD2CSV(
    const char *filename,
    int opt_header,
    rrd_output_callback_t cb,
    void *user,
    int opt_mode,
    char *opt_cf)
{
    unsigned int i, ii, ix, iii = 0;
    time_t    now;
    rrd_value_t my_cdp;
    off_t     rra_base, rra_start, rra_next;
    rrd_file_t *rrd_file;
    rrd_t     rrd;


//These two macros are local defines to clean up visible code from its redundancy
//and make it easier to read.
#define CB_PUTS(str)                                            \
    do {							\
        size_t len = strlen(str);				\
								\
        if (cb((str), len, user) != len)                        \
            goto err_out;                                       \
    } while (0);
#define CB_FMTS(...) do {                                       \
    char buffer[256];                                           \
    rrd_snprintf (buffer, sizeof(buffer), __VA_ARGS__);         \
    CB_PUTS (buffer);                                           \
    } while (0)
//These macros are to be undefined at the end of this function

    //Check if we got a (valid) callback method
    if (!cb) {
        return (-1);
    }

    rrd_init(&rrd);

    rrd_file = rrd_open(filename, &rrd, RRD_READONLY | RRD_LOCK |
                                        RRD_READAHEAD);
    if (rrd_file == NULL) {
        rrd_free(&rrd);
        return (-1);
    }

    // Added CSV format output

    int data_source = CF_AVERAGE;

    if (opt_cf != NULL) {
        if(strcmp(opt_cf, "max") == 0) {
            data_source = CF_MAXIMUM;
        }
        else if (strcmp(opt_cf, "min") == 0) {
            data_source = CF_MINIMUM;
        }
        else if (strcmp(opt_cf, "last") == 0) {
            data_source = CF_LAST;
        }
    }

    CB_FMTS("utimestamp;");
    for (i = 0; i < rrd.stat_head->ds_cnt; i++) {
        CB_FMTS("%s ", rrd.ds_def[i].ds_nam);
        CB_FMTS("(%s);", rrd.ds_def[i].dst);
    }
    CB_FMTS("\n");

    rra_base = rrd_file->header_len;
    rra_next = rra_base;


    for (i = 0; i < rrd.stat_head->rra_cnt; i++) {

        long      timer = 0;

        rra_start = rra_next;
        rra_next += (rrd.stat_head->ds_cnt
                     * rrd.rra_def[i].row_cnt * sizeof(rrd_value_t));


        if (rrd_cf_conv(rrd.rra_def[i].cf_nam) == data_source) {
            rrd_seek(rrd_file, (rra_start + (rrd.rra_ptr[i].cur_row + 1)
                                * rrd.stat_head->ds_cnt
                                * sizeof(rrd_value_t)), SEEK_SET);
            timer = -(long)(rrd.rra_def[i].row_cnt - 1);
            ii = rrd.rra_ptr[i].cur_row;
            for (ix = 0; ix < rrd.rra_def[i].row_cnt; ix++) {
                ii++;
                if (ii >= rrd.rra_def[i].row_cnt) {
                    rrd_seek(rrd_file, rra_start, SEEK_SET);
                    ii = 0; /* wrap if max row cnt is reached */
                }
                now = (rrd.live_head->last_up
                       - rrd.live_head->last_up
                       % (rrd.rra_def[i].pdp_cnt * rrd.stat_head->pdp_step))
                    + (timer * (long)rrd.rra_def[i].pdp_cnt * (long)rrd.stat_head->pdp_step);

                timer++;
                CB_FMTS("%lld;",  (long long int) now);
                for (iii = 0; iii < rrd.stat_head->ds_cnt; iii++) {
                    rrd_read(rrd_file, &my_cdp, sizeof(rrd_value_t) * 1);
                    if (isnan(my_cdp)) {
                        // ignore empty data
                        CB_PUTS(";");
                    } else {
                        CB_FMTS("%0.6f;", my_cdp);
                    }
                }
                CB_PUTS("\n");
            }
        }
    }
        
    rrd_free(&rrd);

    return rrd_close(rrd_file);

err_out:
    rrd_set_error("error writing output file: %s", rrd_strerror(errno));
    rrd_free(&rrd);
    rrd_close(rrd_file);
    return (-1);

//Undefining the previously defined shortcuts
//See start of this function
#undef CB_PUTS
#undef CB_FMTS
//End of macro undefining

}

int rrd_dump_opt_r(
    const char *filename,
    char *outname,
    int opt_noheader,
    int opt_mode,
    char *opt_cf)
{
    FILE     *out_file;
    int       res;

    out_file = NULL;
    if (outname) {
        if (!(out_file = fopen(outname, "w"))) {
            return (-1);
        }
    } else {
        out_file = stdout;
    }

    res = RRD2CSV(filename, opt_noheader, printf, (void *)out_file, opt_mode, opt_cf);

    if (fflush(out_file) != 0) {
        rrd_set_error("error flushing output: %s", rrd_strerror(errno));
        res = -1;
    }
    if (out_file != stdout) {
        fclose(out_file);
        if (res != 0)
            unlink(outname);
    }

    return res;
}


int rrd_csv(
    int argc,
    char **argv)
{
    int       opt;
    struct optparse_long longopts[] = {
        {"daemon",    'd', OPTPARSE_REQUIRED},
        {"header",    'h', OPTPARSE_REQUIRED},
        {"no-header", 'n', OPTPARSE_NONE},
        {"csv",       'c', OPTPARSE_NONE},
        {"cf",       'f', OPTPARSE_REQUIRED},
        {0},
    };
    struct optparse options;
    int       rc;
    /** 
     * 0 = no header
     * 1 = dtd header
     * 2 = xsd header
     */
    int       opt_header = 1;
    int       opt_mode = 0;
    char     *opt_daemon = NULL;
    char     *opt_cf = NULL;

    /* init rrd clean */

    optparse_init(&options, argc, argv);
    while ((opt = optparse_long(&options, longopts, NULL)) != -1) {
        switch (opt) {
        case 'd':
            if (opt_daemon != NULL) {
                    free (opt_daemon);
            }
            opt_daemon = strdup(options.optarg);
            if (opt_daemon == NULL)
            {
                rrd_set_error ("strdup failed.");
                return (-1);
            }
            break;
        case 'f':
            if (opt_cf != NULL) {
                    free (opt_cf);
            }
            opt_cf = strdup(options.optarg);
            if (opt_cf == NULL)
            {
                rrd_set_error ("strdup failed.");
                return (-1);
            }
            break;

        case 'n':
           opt_header = 0;
           break;
        case 'c':
           opt_mode = 1;
           break;

        case 'h':
       if (strcmp(options.optarg, "dtd") == 0) {
        opt_header = 1;
       } else if (strcmp(options.optarg, "xsd") == 0) {
        opt_header = 2;
       } else if (strcmp(options.optarg, "none") == 0) {
        opt_header = 0;
       }
       break;

        default:
            rrd_set_error("usage rrdtool %s [--header|-h {none,xsd,dtd}]\n"
                          "[--no-header|-n]\n"
                          "[--csv|-c]\n"
                          "[--cf|-f avg|max|min|last]\n"
                          "[--daemon|-d address]\n"
                          "file.rrd [file.xml]", options.argv[0]);
            if (opt_daemon != NULL) {
                free(opt_daemon);
            }
            if (opt_cf != NULL) {
                free(opt_cf);
            }
            return (-1);
            break;
        }
    } /* while (opt != -1) */

    if ((options.argc - options.optind) < 1 || (options.argc - options.optind) > 2) {
        rrd_set_error("usage rrdtool %s [--header|-h {none,xsd,dtd}]\n"
                      "[--no-header|-n]\n"
                      "[--csv|-c]\n"
                      "[--cf|-f avg|max|min|last]\n"
                      "[--daemon|-d address]\n"
                       "file.rrd [file.xml]", options.argv[0]);
        if (opt_daemon != NULL) {
            free(opt_daemon);
        }
        return (-1);
    }

    if ((options.argc - options.optind) == 2) {
        rc = rrd_dump_opt_r(options.argv[options.optind], options.argv[options.optind + 1], opt_header, opt_mode, opt_cf);
    } else {
        rc = rrd_dump_opt_r(options.argv[options.optind], NULL, opt_header, opt_mode, opt_cf);
    }

    return rc;
}
int main(int argc,char **argv) {
    if (argc == 1) {
        PrintUsage("");
        return 0;
    }

    rrd_csv(argc,argv);

    return 0;
}
