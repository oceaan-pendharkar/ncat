#ifndef OPEN_H
#define OPEN_H

#include <arpa/inet.h>

int open_keyboard(void);
int open_stdout(void);
int open_stderr(void);
int open_file(const char *path, int flags, int permissions, int *err);
int open_fifo(const char *path, int flags, mode_t permissions, int *err);
int open_domain_socket_client(const char *path, int *err);
int open_domain_socket_server(const char *path, int backlog, int *err);
int open_network_socket_client(const char *address, in_port_t port, int *err);
int open_network_socket_server(const char *address, in_port_t port, int backlog, int *err);

#endif    // OPEN_H
