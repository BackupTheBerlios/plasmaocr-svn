LIBSRC:=bitmaps.c \
	chaincode.c \
	core.c \
	editdist.c \
	io.c \
	library.c \
	linewise.c \
	pattern.c \
	pnm.c \
	rle.c \
	shiftcut.c \
	testing.c \
	thinning.c \
	wordcut.c

CFLAGS:=-pipe -g -Wmissing-prototypes -Wall
CC=gcc
LINK=gcc
BASEOBJDIR:=obj
OBJDIR:=$(BASEOBJDIR)/nontest
TESTDIR:=$(BASEOBJDIR)/test

#set to `y' for some mad ANSI C compatibility options
MANIAC:=n
    


DEPGEN:=-MMD
MANIACFLAGS:= \
       -Wall -Wshadow -pedantic-errors \
       -Wpointer-arith -Waggregate-return \
       -Wstrict-prototypes -Wmissing-declarations \
       -Wlong-long -Winline -Wredundant-decls -Wcast-qual -Wcast-align \
       -D__STRICT_ANSI__

LIBOBJ:=$(LIBSRC:%.c=$(OBJDIR)/%.o)
TESTOBJ:=$(LIBSRC:%.c=$(TESTDIR)/%.o)


ifeq ($(MANIAC),y)
    CFLAGS+=$(MANIACFLAGS)
endif

LIBDEPS:=$(LIBOBJ:%.o=%.d)
TESTDEPS:=$(TESTOBJ:%.o=%.d)


# The rules

.PHONY: all clean rebuild test

all: coldplasma orf2pjf librepl libedit

coldplasma: $(LIBOBJ) $(OBJDIR)/main.o
	$(LINK) $^ $(LDFLAGS) -o $@

orf2pjf: $(LIBOBJ) $(OBJDIR)/orf2pjf.o
	$(LINK) $^ $(LDFLAGS) -o $@

librepl: $(LIBOBJ) $(OBJDIR)/librepl.o
	$(LINK) $^ $(LDFLAGS) -o $@

libedit: $(LIBOBJ) $(OBJDIR)/libedit.o
	$(LINK) $^ $(LDFLAGS) -o $@ `pkg-config --libs gtk+-2.0`

$(TESTDIR)/test: $(TESTOBJ)
	$(LINK) $^ $(LDFLAGS) -o $@

$(OBJDIR)/libedit.o $(OBJDIR)/libedit.d: libedit.c Makefile
	@mkdir -p $(OBJDIR)
	$(CC) -c $(DEPGEN) $(CFLAGS) $< -o $(OBJDIR)/libedit.o `pkg-config --cflags gtk+-2.0` 

$(TESTOBJ): $(TESTDIR)/%.o: %.c Makefile
	@mkdir -p $(TESTDIR)
	$(CC) -DTESTING -c $(DEPGEN) $(CFLAGS) $< -o $(TESTDIR)/$*.o

$(OBJDIR)/%.o: %.c Makefile
	@mkdir -p $(OBJDIR)
	$(CC) -c $(DEPGEN) $(CFLAGS) $< -o $(OBJDIR)/$*.o
	

clean:
	rm -f $(LIBOBJ) $(TESTOBJ) $(LIBDEPS) $(TESTDEPS) coldplasma libedit \
	$(TESTDIR)/test $(OBJDIR)/main.d $(OBJDIR)/main.o \
	$(OBJDIR)/libedit.d $(OBJDIR)/libedit.o \
	$(OBJDIR)/orf2pjf.d $(OBJDIR)/orf2pjf.o \
	$(OBJDIR)/librepl.d $(OBJDIR)/librepl.o
	if [ -d $(TESTDIR) ]; then rmdir $(TESTDIR); fi
	if [ -d $(OBJDIR) ]; then rmdir $(OBJDIR); fi
	if [ -d $(BASEOBJDIR) ]; then rmdir $(BASEOBJDIR); fi

rebuild: clean $(TARGET)

test: $(TESTDIR)/test $(TESTOBJ)
	$<

ifneq ($(MAKECMDGOALS),clean)
  ifneq ($(MAKECMDGOALS),rebuild)
    -include $(LIBDEPS) $(TESTDEPS)
  endif
endif
