#ifndef COPY_H
#define COPY_H

#include <stddef.h>
#include <unistd.h>

ssize_t copy(int fd_in, int fd_out, size_t size, int *err);

#endif    // COPY_H
