/*
 * Copyright (c) 2020, Michael Santos <michael.santos@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <unistd.h>

#define CLOSEFROM_VERSION "0.2.0"

#if defined(__FreeBSD__) || defined(__OpenBSD__)
#else
static int closefrom(int lowfd);
#endif

static void usage(void);

extern char *__progname;

int main(int argc, char *argv[]) {
  int lowfd;
  char *endptr;
#if defined(__OpenBSD__)
  int rv;
#endif

  /* 0: progname
   * 1: lowfd
   * 2: arg0
   * 3..n: arg1..argn
   */
  if (argc < 3)
    usage();

  errno = 0;
  lowfd = (int)strtol(argv[1], &endptr, 10);

  if (errno != 0)
    err(111, "strtol: %s", argv[1]);

  if (endptr == argv[1] || *endptr != '\0' || lowfd < 0)
    usage();

#if defined(__OpenBSD__)
  while (((rv = closefrom(lowfd)) < 0) && errno == EINTR)
    ;
  /* documented errors are EINTR and EBADF */
  if (rv < 0 && errno != EBADF)
    err(111, "closefrom");
#elif defined(__FreeBSD__)
  closefrom(lowfd);
#else
  if (closefrom(lowfd) < 0)
    err(111, "closefrom");
#endif

  (void)execvp(argv[2], argv + 2);

  err(111, "execvp");
}

#if defined(__FreeBSD__) || defined(__OpenBSD__)
#else
static int closefrom(int lowfd) {
  struct rlimit rl = {0};
  int fd;

  if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
    return -1;

  for (fd = rl.rlim_cur; fd >= lowfd; fd--) {
    if (fcntl(fd, F_GETFD, 0) < 0)
      continue;

    if (close(fd) < 0)
      return -1;
  }

  return 0;
}
#endif

static void usage(void) {
  (void)fprintf(stderr,
                "%s %s\n"
                "usage: %s <fd> <cmd> <...>\n",
                __progname, CLOSEFROM_VERSION, __progname);
  exit(1);
}
