#include "../include/open.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

static void setup_address(struct sockaddr_storage *addr, socklen_t *addr_len, const char *address, in_port_t port, int *err);

int open_keyboard(void)
{
    return STDIN_FILENO;
}

int open_stdout(void)
{
    return STDOUT_FILENO;
}

int open_stderr(void)
{
    return STDERR_FILENO;
}

int open_file(const char *path, int flags, int permissions, int *err)
{
    int retval;

    retval = open(path, flags, permissions);

    if(retval < 0)
    {
        *err = errno;
    }

    return retval;
}

int open_fifo(const char *path, int flags, mode_t permissions, int *err)
{
    int result;

    errno  = 0;
    result = mkfifo(path, permissions);

    if(result < 0 && errno != EEXIST)
    {
        *err = errno;
        goto done;
    }

    result = open_file(path, flags, 0, err);
done:

    return result;
}

int open_domain_socket_client(const char *path, int *err)
{
    int                fd;
    int                result;
    struct sockaddr_un addr;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);    // NOLINT(android-cloexec-socket)

    if(fd == -1)
    {
        *err = errno;
        goto done;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';
    result                                   = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));

    if(result == -1)
    {
        *err = errno;
        close(fd);
        fd = -1;
    }

done:
    return fd;
}

int open_domain_socket_server(const char *path, int backlog, int *err)
{
    int                server_fd;
    int                client_fd;
    int                result;
    struct sockaddr_un addr;

    client_fd = -1;
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);    // NOLINT(android-cloexec-socket)

    if(server_fd == -1)
    {
        *err = errno;
        goto done;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';
    result                                   = bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));

    if(result == -1)
    {
        *err = errno;
        goto done;
    }

    result = listen(server_fd, backlog);

    if(result == -1)
    {
        *err = errno;
        goto done;
    }

    client_fd = accept(server_fd, NULL, 0);

    if(client_fd == -1)
    {
        *err = errno;
    }

done:
    if(server_fd != -1)
    {
        close(server_fd);
    }

    return client_fd;
}

int open_network_socket_client(const char *address, in_port_t port, int *err)
{
    int                     fd;
    int                     result;
    struct sockaddr_storage addr;
    socklen_t               addr_len;

    fd = socket(AF_INET, SOCK_STREAM, 0);    // NOLINT(android-cloexec-socket)

    if(fd == -1)
    {
        *err = errno;
        goto done;
    }

    memset(&addr, 0, sizeof(addr));
    setup_address(&addr, &addr_len, address, port, err);
    result = connect(fd, (const struct sockaddr *)&addr, addr_len);

    if(result == -1)
    {
        *err = errno;
        close(fd);
        fd = -1;
    }

done:
    return fd;
}

int open_network_socket_server(const char *address, in_port_t port, int backlog, int *err)
{
    int                     server_fd;
    int                     client_fd;
    int                     result;
    struct sockaddr_storage addr;
    socklen_t               addr_len;

    client_fd = -1;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);    // NOLINT(android-cloexec-socket)

    if(server_fd == -1)
    {
        *err = errno;
        goto done;
    }

    memset(&addr, 0, sizeof(addr));
    setup_address(&addr, &addr_len, address, port, err);
    result = bind(server_fd, (struct sockaddr *)&addr, addr_len);

    if(result == -1)
    {
        *err = errno;
        goto done;
    }

    result = listen(server_fd, backlog);

    if(result == -1)
    {
        *err = errno;
        goto done;
    }

    client_fd = accept(server_fd, NULL, 0);

    if(client_fd == -1)
    {
        *err = errno;
    }

done:
    if(server_fd != -1)
    {
        close(server_fd);
    }

    return client_fd;
}

static void setup_address(struct sockaddr_storage *addr, socklen_t *addr_len, const char *address, in_port_t port, int *err)
{
    in_port_t net_port;

    *addr_len = 0;
    net_port  = htons(port);
    memset(addr, 0, sizeof(*addr));

    if(inet_pton(AF_INET, address, &(((struct sockaddr_in *)addr)->sin_addr)) == 1)
    {
        struct sockaddr_in *ipv4_addr;

        ipv4_addr           = (struct sockaddr_in *)addr;
        addr->ss_family     = AF_INET;
        ipv4_addr->sin_port = net_port;
        *addr_len           = sizeof(struct sockaddr_in);
    }
    else if(inet_pton(AF_INET6, address, &(((struct sockaddr_in6 *)addr)->sin6_addr)) == 1)
    {
        struct sockaddr_in6 *ipv6_addr;

        ipv6_addr            = (struct sockaddr_in6 *)addr;
        addr->ss_family      = AF_INET6;
        ipv6_addr->sin6_port = net_port;
        *addr_len            = sizeof(struct sockaddr_in6);
    }
    else
    {
        fprintf(stderr, "%s is not an IPv4 or an IPv6 address\n", address);
        *err = errno;
    }
}
