# SYNOPSIS

closefrom *fd* *cmd* *arg* *...*

# DESCRIPTION

closefrom - close(2) a range of file descriptors before exec(2)

`closefrom` closes all file descriptors numbered fd and higher before
executing a program.

`exec(2)`ing a file can unintentionally leak file descriptors to
the new process image. These file descriptors may provide unexpected
[capabilities](https://www.freebsd.org/cgi/man.cgi?capsicum(4)) to
the process.

`closefrom` is run as a part of an exec chain, closing many descriptors
similar to the [closefrom(2)](https://man.openbsd.org/closefrom) system
call, before executing the target process.

# EXAMPLES

## [ucspi-unix](https://github.com/bruceg/ucspi-unix)

  `ucspi-unix` is an example of the "Defer to Kernel"
  privilege separation model in [Secure Design
  Patterns](https://resources.sei.cmu.edu/asset_files/TechnicalReport/2009_005_001_15110.pdf).

  The guarantees are broken because `ucspi-unix` [leaks the listening
  socket](https://github.com/bruceg/ucspi-unix/pull/2) to the application
  subprocess. The application subprocess can race the server in accepting
  new connections and bypass unix socket permissions and socket credential
  checks.

~~~ C
#include <stdio.h>
#include <unistd.h>

#include <err.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

/* cc -g -Wall -o accept accept.c */

int main(int argc, char *argv[]) {
  int fd;
  struct sockaddr_un addr;
  socklen_t addrlen = sizeof(addr);

  for (;;) {
    fd = accept(3, (struct sockaddr *)&addr, &addrlen);
    if (fd < 0) {
      warn("accept");
      continue;
    }
    (void)write(fd, "12345678\n", 9);
    (void)close(fd);
  }
}
~~~

~~~
# run unixserver as root
sudo unixserver -m 077 /tmp/test.sock -- setuidgid nobody ./accept

# connect to the socket
$ sudo nc -U /tmp/test.sock
$ sudo nc -U /tmp/test.sock
$ sudo nc -U /tmp/test.sock
12345678

# with closefrom
sudo unixserver -m 077 /tmp/test.sock -- closefrom 3 setuidgid nobody ./accept
accept: accept: Bad file descriptor
~~~

## LXC

## shell

This example opens and leaks a file descriptor to `cat(1)`:

~~~ shell
#!/bin/bash

exec 9</dev/null
exec $@
~~~

~~~
$ leakfd ls -al /proc/self/fd
total 0
dr-x------. 2 msantos msantos  0 Aug 28 09:28 .
dr-xr-xr-x. 9 msantos msantos  0 Aug 28 09:28 ..
lrwx------. 1 msantos msantos 64 Aug 28 09:28 0 -> /dev/pts/19
lrwx------. 1 msantos msantos 64 Aug 28 09:28 1 -> /dev/pts/19
lrwx------. 1 msantos msantos 64 Aug 28 09:28 2 -> /dev/pts/19
lr-x------. 1 msantos msantos 64 Aug 28 09:28 3 -> /proc/32048/fd
lr-x------. 1 msantos msantos 64 Aug 28 09:28 9 -> /dev/null

$ leakfd closefrom 3 ls -al /proc/self/fd
total 0
dr-x------. 2 msantos msantos  0 Aug 28 09:29 .
dr-xr-xr-x. 9 msantos msantos  0 Aug 28 09:29 ..
lrwx------. 1 msantos msantos 64 Aug 28 09:29 0 -> /dev/pts/19
lrwx------. 1 msantos msantos 64 Aug 28 09:29 1 -> /dev/pts/19
lrwx------. 1 msantos msantos 64 Aug 28 09:29 2 -> /dev/pts/19
lr-x------. 1 msantos msantos 64 Aug 28 09:29 3 -> /proc/32058/fd
~~~

# OPTIONS

None.

# BUILDING

    make

    # statically linked executable
    ./musl-make

    # some versions of glibc support closefrom(2)
    CLOSEFROM_CFLAGS="-DHAVE_CLOSEFROM" make

    # run tests
    make clean all test

# ALTERNATIVES

* bash

~~~ shell
#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

NOFILE="$(ulimit -n)"
LOWFD="$1"
shift
for fd in $(seq "$LOWFD" "$NOFILE"); do
  eval "exec $fd>&-"
done
exec $@
~~~

* [fdclose](http://skarnet.org./software/execline/fdclose.html)

# SEE ALSO

_close_(2), _closefrom(2)_, _exec(3)_
