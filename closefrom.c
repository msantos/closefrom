/*
 * Copyright (c) 2020-2022, Michael Santos <michael.santos@gmail.com>
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
#include <stdnoreturn.h>
#include <sys/resource.h>
#include <unistd.h>

#if defined(__FreeBSD__) || defined(__OpenBSD__)
#define HAVE_CLOSEFROM
#endif

#if !defined(HAVE_CLOSEFROM)
#include <dirent.h>
#include <sys/types.h>

static int isnum(const char *);
static int closefrom(int lowfd);
static int closefrom_all(int lowfd);
#endif

static noreturn void usage(void);

extern char *__progname;

#define CLOSEFROM_VERSION "1.0.0"

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
#elif defined(HAVE_CLOSEFROM)
  closefrom(lowfd);
#else
  if (closefrom(lowfd) < 0)
    err(111, "closefrom");
#endif

  (void)execvp(argv[2], argv + 2);

  err(127, "%s", argv[2]);
}

#if !defined(HAVE_CLOSEFROM)
static int closefrom(int lowfd) {
  DIR *dp;
  int dfd;
  struct dirent *de;
  int fd;

  /* The opendir() function sets the close-on-exec flag for the file
   * descriptor underlying the DIR *. */
  dp = opendir("/dev/fd");
  if (dp == NULL) {
    return closefrom_all(lowfd);
  }

  dfd = dirfd(dp);
  if (dfd == -1) {
    (void)closedir(dp);
    return closefrom_all(lowfd);
  }

  while ((de = readdir(dp)) != NULL) {
    if (!isnum(de->d_name))
      continue;

    fd = atoi(de->d_name);

    if (fd < lowfd || fd == dfd)
      continue;

    if (close(fd) == -1)
      return -1;
  }

  return 0;
}

static int isnum(const char *s) {
  const char *p;

  for (p = s; *p != '\0'; p++) {
    if (*p < '0' || *p > '9')
      return 0;
  }

  return 1;
}

static int closefrom_all(int lowfd) {
  struct rlimit rl = {0};
  int fd;

  if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
    return -1;

  for (fd = rl.rlim_cur; fd >= lowfd; fd--) {
    if (fcntl(fd, F_GETFD, 0) == -1)
      continue;

    if (close(fd) == -1)
      return -1;
  }

  return 0;
}
#endif

static noreturn void usage(void) {
  (void)fprintf(stderr,
                "%s %s\n"
                "usage: %s <fd> <cmd> <...>\n",
                __progname, CLOSEFROM_VERSION, __progname);
  exit(1);
}
