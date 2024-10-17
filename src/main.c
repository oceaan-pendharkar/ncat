#include "../include/copy.h"
#include "../include/open.h"
#include <getopt.h>
#include <open.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>

struct options
{
    bool  in;
    bool  out;
    bool  err;
    char *infile;
    char *outfile;
    char *infifo;
    char *outfifo;
    char *indomain;
    char *outdomain;
    char *inipv4address;
    char *outipv4address;
};

static void           parse_arguments(int argc, char *argv[], struct options *opts);
static void           check_arguments(const char *binary_name, const struct options *opts);
static void           check_inputs(const char *binary_name, const struct options *opts);
static void           check_outputs(const char *binary_name, const struct options *opts);
_Noreturn static void usage(const char *program_name, int exit_code, const char *message);
static int            get_input(const struct options *opts, int *err);
static int            get_output(const struct options *opts, int *err);
static void           close_input(const struct options *opts, int fd);
static void           close_output(const struct options *opts, int fd);

#define BUFSIZE 128
#define PORT 9999
#define BACKLOG 5
#define MISSING_OPTION_MESSAGE_LEN 29
#define UNKNOWN_OPTION_MESSAGE_LEN 24

int main(int argc, char *argv[])
{
    struct options opts;
    int            in_fd;
    int            out_fd;
    int            err;
    ssize_t        result;

    memset(&opts, 0, sizeof(opts));
    parse_arguments(argc, argv, &opts);
    check_arguments(argv[0], &opts);

    /*
    printf("in %d\n", opts.in);
    printf("out %d\n", opts.out);
    printf("err %d\n", opts.err);
    printf("infile %s\n", opts.infile);
    printf("outfile %s\n", opts.outfile);
    printf("infifo %s\n", opts.infifo);
    printf("outfifo %s\n", opts.outfifo);
    printf("indomain %s\n", opts.indomain);
    printf("outdomain %s\n", opts.outdomain);
*/

    err   = 0;
    in_fd = get_input(&opts, &err);

    if(in_fd < 0)
    {
        const char *msg;

        msg = strerror(err);
        printf("Error opening input: %s\n", msg);
        goto err_in;
    }

    out_fd = get_output(&opts, &err);

    if(out_fd < 0)
    {
        const char *msg;

        msg = strerror(err);
        printf("Error opening output: %s\n", msg);
        goto err_out;
    }

    result = copy(in_fd, out_fd, BUFSIZE, &err);

    if(result == -1)
    {
        perror("malloc");
    }
    else if(result == -2)
    {
        perror("read");
    }
    else if(result == -3)
    {
        perror("write");
    }

    close_output(&opts, out_fd);

err_out:
    close_input(&opts, in_fd);

err_in:

    return EXIT_SUCCESS;
}

static void parse_arguments(int argc, char *argv[], struct options *opts)
{
    static struct option long_options[] = {
        {"in",        no_argument,       NULL, 'i'},
        {"out",       no_argument,       NULL, 'o'},
        {"err",       no_argument,       NULL, 'e'},
        {"infile",    required_argument, NULL, 'f'},
        {"outfile",   required_argument, NULL, 'F'},
        {"infifo",    required_argument, NULL, 'p'},
        {"outfifo",   required_argument, NULL, 'P'},
        {"indomain",  required_argument, NULL, 'd'},
        {"outdomain", required_argument, NULL, 'D'},
        {"inipv4",    required_argument, NULL, 'n'},
        {"outipv4",   required_argument, NULL, 'N'},
        {"help",      no_argument,       NULL, 'h'},
        {NULL,        0,                 NULL, 0  }
    };
    int opt;

    opterr = 0;

    while((opt = getopt_long(argc, argv, "hioef:F:p:P:d:D:n:N:", long_options, NULL)) != -1)
    {
        switch(opt)
        {
            case 'i':
            {
                opts->in = true;
                break;
            }
            case 'o':
            {
                opts->out = true;
                break;
            }
            case 'e':
            {
                opts->err = true;
                break;
            }
            case 'f':
            {
                opts->infile = optarg;
                break;
            }
            case 'F':
            {
                opts->outfile = optarg;
                break;
            }
            case 'p':
            {
                opts->infifo = optarg;
                break;
            }
            case 'P':
            {
                opts->outfifo = optarg;
                break;
            }
            case 'd':
            {
                opts->indomain = optarg;
                break;
            }
            case 'D':
            {
                opts->outdomain = optarg;
                break;
            }
            case 'n':
            {
                opts->inipv4address = optarg;
                break;
            }
            case 'N':
            {
                opts->outipv4address = optarg;
                break;
            }
            case 'h':
            {
                usage(argv[0], EXIT_SUCCESS, NULL);
            }
            case '?':
            {
                if(optopt == 'f' || optopt == 'F' || optopt == 'p' || optopt == 'P' || optopt == 'd' || optopt == 'D' || optopt == 'n' || optopt == 'N')
                {
                    char message[MISSING_OPTION_MESSAGE_LEN];

                    snprintf(message, sizeof(message), "Option '-%c' requires a path.", optopt);
                    usage(argv[0], EXIT_FAILURE, message);
                }
                else
                {
                    char message[UNKNOWN_OPTION_MESSAGE_LEN];

                    snprintf(message, sizeof(message), "Unknown option '-%c'.", optopt);
                    usage(argv[0], EXIT_FAILURE, message);
                }
            }
            default:
            {
                usage(argv[0], EXIT_FAILURE, NULL);
            }
        }
    }
}

static void check_arguments(const char *binary_name, const struct options *opts)
{
    check_inputs(binary_name, opts);
    check_outputs(binary_name, opts);
}

static void check_inputs(const char *binary_name, const struct options *opts)
{
    int in_count;

    in_count = 0;

    if(opts->in)
    {
        in_count++;
    }

    if(opts->infile)
    {
        in_count++;
    }

    if(opts->infifo)
    {
        in_count++;
    }

    if(opts->indomain)
    {
        in_count++;
    }

    if(opts->inipv4address)
    {
        in_count++;
    }

    if(in_count == 0)
    {
        usage(binary_name, EXIT_FAILURE, "an input is required");
    }

    if(in_count > 1)
    {
        usage(binary_name, EXIT_FAILURE, "only one input can be specified");
    }
}

static void check_outputs(const char *binary_name, const struct options *opts)
{
    int out_count;

    out_count = 0;

    if(opts->out)
    {
        out_count++;
    }

    if(opts->err)
    {
        out_count++;
    }

    if(opts->outfile)
    {
        out_count++;
    }

    if(opts->outfifo)
    {
        out_count++;
    }

    if(opts->outdomain)
    {
        out_count++;
    }

    if(opts->outipv4address)
    {
        out_count++;
    }

    if(out_count == 0)
    {
        usage(binary_name, EXIT_FAILURE, "an output is required");
    }

    if(out_count > 1)
    {
        usage(binary_name, EXIT_FAILURE, "only one output can be specified");
    }
}

_Noreturn static void usage(const char *program_name, int exit_code, const char *message)
{
    if(message)
    {
        fprintf(stderr, "%s\n", message);
    }

    fprintf(stderr, "Usage: %s [-h] [-i] [-o] [-e] [-f <path>] [-F <path>] [-p <path>] [-P <path>] [-d <path>] [-D <path>] [-n <address>] [-N <address>]\n", program_name);
    fputs("Options:\n", stderr);
    fputs("  -h, --help                      Display this help message\n", stderr);
    fputs("  -i, --in                        read from the keyboard (stdin)\n", stderr);
    fputs("  -o, --out                       write to the screen (stdout)\n", stderr);
    fputs("  -e, --err                       write to the screen (stderr)\n", stderr);
    fputs("  -f <value>, --infile <path>     read from the file <path>\n", stderr);
    fputs("  -F <value>, --outfile <path>    write to the file <path>\n", stderr);
    fputs("  -p <value>, --infifo <path>     read from the FIFO <path>\n", stderr);
    fputs("  -P <value>, --outfifo <path>    write to the FIFO <path>\n", stderr);
    fputs("  -d <value>, --indomain <path>   read from the domain socket <path>\n", stderr);
    fputs("  -D <value>, --outdomain <path>  write to the domain socket <path>\n", stderr);
    fputs("  -n <value>, --inipv4 <address>  read from the network socket <address>\n", stderr);
    fputs("  -N <value>, --outipv4 <address> write to the network socket <address>\n", stderr);
    exit(exit_code);
}

static int get_input(const struct options *opts, int *err)
{
    int fd;

    if(opts->in)
    {
        fd = open_keyboard();
    }
    else if(opts->infile != NULL)
    {
        fd = open_file(opts->infile, O_RDONLY, 0, err);
    }
    else if(opts->infifo != NULL)
    {
        fd = open_fifo(opts->infifo, O_RDONLY, 0, err);
    }
    else if(opts->indomain != NULL)
    {
        fd = open_domain_socket_server(opts->indomain, BACKLOG, err);
    }
    else if(opts->inipv4address != NULL)
    {
        fd = open_network_socket_server(opts->inipv4address, PORT, BACKLOG, err);
    }
    else
    {
        fd = -1;
    }

    return fd;
}

static int get_output(const struct options *opts, int *err)
{
    int fd;

    if(opts->out)
    {
        fd = open_stdout();
    }
    else if(opts->err)
    {
        fd = open_stderr();
    }
    else if(opts->outfile != NULL)
    {
        fd = open_file(opts->outfile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR, err);
    }
    else if(opts->outfifo != NULL)
    {
        fd = open_fifo(opts->outfifo, O_WRONLY, S_IRUSR | S_IWUSR, err);
    }
    else if(opts->outdomain != NULL)
    {
        fd = open_domain_socket_client(opts->outdomain, err);
    }
    else if(opts->outipv4address != NULL)
    {
        fd = open_network_socket_client(opts->outipv4address, PORT, err);
    }
    else
    {
        fd = -1;
    }

    return fd;
}

static void close_input(const struct options *opts, int fd)
{
    if(opts->infile != NULL || opts->infifo != NULL || opts->indomain != NULL || opts->inipv4address != NULL)
    {
        const char *path;

        close(fd);
        path = NULL;

        if(opts->infifo)
        {
            path = opts->infifo;
        }
        else if(opts->indomain)
        {
            path = opts->indomain;
        }

        if(path)
        {
            unlink(path);
        }
    }
}

static void close_output(const struct options *opts, int fd)
{
    if(opts->outfile != NULL || opts->outfifo != NULL || opts->outdomain != NULL || opts->outipv4address != NULL)
    {
        close(fd);
    }
}
