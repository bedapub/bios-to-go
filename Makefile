CC = gcc
CCFLAGS = -Wall -Wno-parentheses -Wno-sign-compare -Wno-unknown-pragmas

PROGS = example

B = ./bin
O = ./obj
K = ./kern
C = ./src
S = ./scripts

all: $(PROGS)
	chmod +x $S/rmcr.pl && /bin/rm -f $B/rmcr && /bin/cp -pr $S/rmcr.pl $B/rmcr

clean:
	for s in $(PROGS) ; do \
		/bin/rm -f $B/$$s ; \
		/bin/rm -f $B/rmcr ; \
	done

# C programs - example
example: $C/example.c $K/plabla.c $K/linestream.c $K/rofutil.c $K/array.c \
	$K/format.c $K/log.c $K/arg.c $K/hlrmisc.c
	@-/bin/rm -f $(B)/example
	$(CC) $(CCFLAGS) $C/example.c -o $B/example $K/plabla.c $K/linestream.c $K/rofutil.c \
	$K/array.c $K/format.c $K/log.c $K/arg.c $K/hlrmisc.c -lm -I$K


# Scripts
rmcr: $S/rmcr.pl
	cp -p $S/rmcr.pl $B/rmcr
	chmod +x $B/rmcr
