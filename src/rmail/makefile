DEFINES=-DHAVE_STRING_H -DREGEX_MALLOC -DSTDC_HEADERS -DUSE_REGEXP
INCLUDES=../rx-1.5/regex
VPATH=../rx-1.5/regex
ifeq ($(COMPILER),IBM)
SYSINCLUDE=$(INCLUDE)
EXT=obj
CC=icc
ifdef DEBUG
CFLAGS=/Q+ /Ti+ /Wpro- /Wcnd- $(DEFINES) $(INCLUDES) /Fo$@ /C+
LFLAGS=/Ti+ /Fe$@
OBJDIR=ibm.deb
else
CFLAGS=/Q+ /G5 /Gi+ /O+ /Ol+ /Oi+ /Wpro- /Wcnd- $(DEFINES) $(INCLUDES) /Fo$@ /C+
#/G5               Generate code optimized for use on a Pentium processor.
#/Gi<[+]|->        Generate fast integer code.  Default: /Gi-
#/O<[+]|->         Optimize generated code.  Default: /O-
#/Oi<[+]|-|value>  /Oi+: Turn on function inlining.  Default: /Oi- if /O-
#/Ol<[+]|->        Invoke intermediate code linker.  Default: /Ol-
LFLAGS=/Fe$@
OBJDIR=ibm
endif
else
SYSINCLUDE=$(C_INCLUDE_PATH)
EXT=o
CC=gcc
ifdef DEBUG
CFLAGS=$(DEFINES) $(INCLUDES) -g -o $@ -c
LFLAGS=-g -o $@
OBJDIR=emx.deb
else
CFLAGS=$(DEFINES) $(INCLUDES) -O2 -o $@ -c
LFLAGS=-O2 -o $@
OBJDIR=emx
endif
endif

OBJ=rmail.$(EXT) rmailidx.$(EXT) regex.$(EXT) showrdx.$(EXT)
OBJLIST=$(addprefix $(OBJDIR)/,$(OBJ))
FILES=$(OBJLIST) makefile

all: $(OBJDIR)/rmail.exe $(OBJDIR)/rmailidx.exe $(OBJDIR)/showrdx.exe

clean:
	rm -f $(OBJDIR)/rmail.exe $(OBJDIR)/rmailidx.exe $(OBJDIR)/showrdx.exe $(OBJDIR)/*.$(EXT) $(OBJDIR)/*.d

$(OBJDIR):
	if not exist $@ md $@

$(OBJDIR)/rmail.exe: $(OBJDIR)/rmail.$(EXT)
	$(CC) $(LFLAGS) $^

$(OBJDIR)/rmailidx.exe: $(OBJDIR)/rmailidx.$(EXT) $(OBJDIR)/rmaillib.$(EXT) $(OBJDIR)/regex.$(EXT)
	$(CC) $(LFLAGS) $^

$(OBJDIR)/rmaillib.$(EXT): rmail.c rmail.h
	$(CC) $(CFLAGS) $<

$(OBJDIR)/showrdx.exe: $(OBJDIR)/showrdx.$(EXT) $(OBJDIR)/rmaillib.$(EXT)
	$(CC) $(LFLAGS) $^

#tag: $(addsuffix .__tag,$(basename $(FILES)))

ifeq ($(COMPILER),IBM)
include $(OBJLIST:.obj=.d)
$(OBJDIR)/%.obj: %.c
	$(CC) $(CFLAGS) -DTEST_LIBRARY $<
else
include $(OBJLIST:.o=.d)
$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -DTEST_LIBRARY $<
endif

$(OBJDIR)/%.d: %.c
	if not exist $(OBJDIR) md $(OBJDIR)
	makedepend -f- $(DEFINES) -I$(subst ;, -I,$(SYSINCLUDE)) -Y $< | sed -e "s@ [a-zA-Z]:/[^ ]*@@g" -e "s@^\(.*\)\.o:@$(OBJDIR)/\1.__tag $(OBJDIR)/\1.d $(OBJDIR)/\1.$(EXT): $< makefile@" -e "s@^#.*@@g" > $@


%.__tag: %.c
ifndef TAG 
	@echo Kein Tag angegeben
	@exit 1
else
	@$(foreach i,$^,call cvscheck $(i) &&)
ifndef CHECK_ONLY
ifdef MOVE_TAG
	@for %i in ($^) do (cvs tag -F $(TAG) %i)
else
	@for %i in ($^) do (cvs tag $(TAG) %i)
endif
endif
endif

makefile.__tag: makefile
ifndef TAG 
	@echo Kein Tag angegeben
	@exit 1
else
	@$(foreach i,$^,call cvscheck $(i) &&)
ifndef CHECK_ONLY
ifdef MOVE_TAG
	@for %i in ($^) do (cvs tag -F $(TAG) %i)
else
	@for %i in ($^) do (cvs tag $(TAG) %i)
endif
endif
endif
