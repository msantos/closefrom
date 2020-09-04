.PHONY: all clean test

PROG=   closefrom
SRCS=   closefrom.c

CFLAGS ?= -D_FORTIFY_SOURCE=2 -O2 -fstack-protector-strong \
					-Wformat -Werror=format-security \
					-pie -fPIE \
					-fno-strict-aliasing \
					-g -Wall -fwrapv -pedantic
LDFLAGS ?= -Wl,-z,relro,-z,now -Wl,-z,noexecstack

CFLAGS += $(CLOSEFROM_CFLAGS)
LDFLAGS += $(CLOSEFROM_LDFLAGS)

all:
	$(CC) $(CFLAGS) -o $(PROG) $(SRCS) $(LDFLAGS)

clean:
	-@rm $(PROG)

test: $(PROG)
	@PATH=.:$(PATH) bats test
