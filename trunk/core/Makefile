TARGET=coldplasma#core_test
SOURCES:=$(wildcard *.c)
CFLAGS:=-pipe -g -Wmissing-prototypes -Wall #-DTESTING
CC=gcc
LINK=gcc
OBJDIR:=obj

#set to `y' for some mad ANSI C compatibility options
MANIAC:=n
    


DEPGEN:=-MMD
MANIACFLAGS:= \
       -Wall -Werror -Wshadow -pedantic-errors \
       -Wpointer-arith -Waggregate-return \
       -Wstrict-prototypes -Wmissing-declarations \
       -Wlong-long -Winline -Wredundant-decls -Wcast-qual -Wcast-align \
       -D__STRICT_ANSI__
       

ifeq ($(MANIAC),y)
    CFLAGS+=$(MANIACFLAGS)
endif

OBJECTS:=$(SOURCES:%.c=$(OBJDIR)/%.o)
DEPS:=$(OBJECTS:%.o=%.d)


# The rules

.PHONY: clean rebuild test

$(TARGET): $(OBJECTS) Makefile
	$(LINK) $(OBJECTS) $(LDFLAGS) -o $(TARGET)
	
$(OBJDIR)/%.o $(OBJDIR)/%.d: %.c Makefile
	@mkdir -p $(OBJDIR)
	$(CC) -c $(DEPGEN) $(CFLAGS) $< -o $(OBJDIR)/$*.o

clean:
	rm -f $(OBJECTS) $(DEPS) $(TARGET)
	rmdir $(OBJDIR)

rebuild: clean $(TARGET)

test: $(TARGET)
	./$(TARGET)

ifneq ($(MAKECMDGOALS),clean)
  ifneq ($(MAKECMDGOALS),rebuild)
    -include $(DEPS)
  endif
endif
