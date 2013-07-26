#############################################################################
# Makefile for SM2AddOn
#############################################################################
#
# SM2ADDON
# Copyright (c) 1996-98 Steffen Siebert (siebert@logware.de)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#############################################################################
#
# Tools needed to compile the DLL's:
#
# 1. IBM Visual Age C++ Compiler 3.0 for OS/2
# 2. GNU make 3.75
# 3. Makedepend
# 4. GNU sed 3.0
#
#############################################################################
#
# It is also possible to use the GNU GCC compiler from the
# EMX 0.9c Package to compile the DLL's. Unfortunatly the
# DLL's don't work correct then (e.g. getenv() doesn't see
# the environment variables). If someone manages to create
# the DLL's with GCC, please mail me the necessary changes.
#
#############################################################################
#
# Usage:
#
# To build the DLL's, just type "make".
#
# You can set several variables to create special versions of the
# DLL's. Each version resides in a separate subdirectory, thus you can
# switch between the different versions without the need to delete the
# object files and DLL's.
#
# Set "DEBUG_LOG=1" to create a version which writes debug information
# into a logfile. Please set DEBUG_FNAME in sm_debug.h to the name and
# path of the logfile you want to create.
#
# Set "DEBUG=1" to create a version with debug information.
#
# SM2AddOn can use a heuristic to determine the language of emails stored
# in RMAIL files. This enables you to store emails in different languages
# into one RMAIL file.
#
# Currently only english and german are supported. If all your emails
# are in the same language, you should disable this feature since it
# slows the indexing process of RMAIL files.
#
# Set "NO_LANGUAGE_DETECTION=1" to disable the detection.
#
#
# You can install each version using the target "install_boot" or "install",
# e.g. "make DEBUG_LOG=1 install"
#
# "install" just copies the compiled DLL's into the path specified in
# INSTALL_DIR. Since the SM/2 DLL's are in use when the WPS or
# SM/2 Server is running, you have to stop SM/2 with "sm2 stop sm2"
# and shutdown the WPS first.
#
# "install_boot" needs the tool emxupd from the EMX runtime package and
# replaces the locked DLL's with the new ones. But you have to reboot to
# get the new DLL's loaded.
#
#############################################################################

#############################################################################
# Configuration
#############################################################################
#
# Please set SM2TOOLKIT to the path of the SM/2 Toolkit Headers:
#
SM2TOOLKIT=\sm2\toolkit\include
#
# Please set INSTALL_DIR to the path where the AddOn DLL's whould
# be installed (LIBPATH must contain the path prior to sm2/dll!)
#
INSTALL_DIR=\sm2\dll
##
#############################################################################

#############################################################################
# No changes should be necessary after this point.
#############################################################################

DEFINES=-DHAVE_STRING_H -DREGEX_MALLOC -DSTDC_HEADERS

INCLUDES=-I$(SM2TOOLKIT) -Izlib

ifdef DEBUG_LOG
DEFINES+=-DDEBUG_LOG_SM2
DIR1=_l
else
DIR1=
endif

ifndef NO_LANGUAGE_DETECTION
DEFINES+=-DUSE_REGEXP
INCLUDES+=-Irx\regex
DIR2=_r
else
DIR2=
endif

DIRS=$(DIR1)$(DIR2)

ifneq ($(COMPILER),EMX)
#############################################################################
# Settings for IBM Visual Age C++ 3.0
#############################################################################
SYSINCLUDE=$(INCLUDE)
EXT=obj
CC=icc
MAKEDEPEND=makedepend -f- -Y -w0
ZLIBCFLAGS=/Q+ /G5 /Gi+ /Ss+ /O+ /Ol+ /Oi+ /Fo$@ /C+
ifdef DEBUG
CFLAGS=/Q+ /Ti+ /Tm+ /Ge- /Ss+ $(DEFINES) $(INCLUDES) /Fo$@ /C+
LDFLAGS=/Ti+ /Fm$(basename $@).map /Fe$@
OBJDIR=ibm$(DIRS).deb
else
CFLAGS=/Q+ /G5 /Ge- /Gi+ /Ss+ /O+ /Ol+ /Oi+ $(DEFINES) $(INCLUDES) /Fo$@ /C+
#/G5               Generate code optimized for use on a Pentium processor.
#/Ge-              Use the version of the runtime library that assumes a
#                  DLL is being built.  Default: /Ge+
#/Gi<[+]|->        Generate fast integer code.  Default: /Gi-
#/Ss<[+]|->        Allow the double slash (//) format to be used for comments.
#                  Default: /Ss-  (C mode only)
#/O<[+]|->         Optimize generated code.  Default: /O-
#/Oi<[+]|-|value>  /Oi+: Turn on function inlining.  Default: /Oi- if /O-
#/Ol<[+]|->        Invoke intermediate code linker.  Default: /Ol-
LDFLAGS=/Fm$(basename $@).map /Fe$@
OBJDIR=ibm$(DIRS)
endif
LINK=icc $(LDFLAGS)
LINKE=icc /Ti+ 
else
#############################################################################
# Settings for EMX 0.9c (DOES NOT WORK!)
#############################################################################
SYSINCLUDE=$(C_INCLUDE_PATH)
EXT=obj
CC=gcc
MAKEDEPEND=gcc -M -MG
ZLIBCFLAGS=-Zomf -Zsys -Zdll -O2 -o $@ -c
ifdef DEBUG
CFLAGS=-Zomf -Zsys -Zdll $(DEFINES) -D__OS2__ $(INCLUDES) -funsigned-char -g -o $@ -c
LDFLAGS=-Zomf -Zsys -Zdll -g -o $@
LDFLAGSE=-Zomf -g -o $@
OBJDIR=emx$(DIRS).deb
else
CFLAGS=-Zomf $(DEFINES) -D__OS2__ $(INCLUDES) -funsigned-char -O2 -o $@ -c
LDFLAGS=-Zcrtdll -Zdll -O2 -o $@
LDFLAGSE=-Zomf -O2 -o $@
OBJDIR=emx$(DIRS)
endif
LINK=gcc $(LDFLAGS)
LINKE=gcc $(LDFLAGSE)
endif
#############################################################################

OBJ=cmplsxrd.$(EXT) smplsxrd.$(EXT) smplsdss.$(EXT) rmail.$(EXT) rmailidx.$(EXT)
ifndef NO_LANGUAGE_DETECTION
OBJ+=regex.$(EXT)
endif
OBJLIST=$(addprefix $(OBJDIR)/,$(OBJ))
LIBOBJ=adler32.$(EXT) compress.$(EXT) crc32.$(EXT) deflate.$(EXT) gzio.$(EXT) \
infblock.$(EXT) infcodes.$(EXT)  inffast.$(EXT) inflate.$(EXT) \
inftrees.$(EXT) infutil.$(EXT) trees.$(EXT) uncompr.$(EXT) zutil.$(EXT)
LIBOBJLIST=$(addprefix $(OBJDIR)/,$(LIBOBJ))
FILES=$(LIBOBJ) makefile
VPATH=rx/regex/ rmail/

all: $(OBJDIR)/ehslssfs.dll $(OBJDIR)/ehslscfs.dll 

$(OBJDIR):
	if not exist $@ md $@

ifdef NO_LANGUAGE_DETECTION
$(OBJDIR)/ehslssfs.dll: $(OBJDIR)/smplsxrd.$(EXT) $(OBJDIR)/smplsdss.$(EXT) $(OBJDIR)/rmail.$(EXT) $(OBJDIR)/rmailidx.$(EXT) smplssrd.def $(LIBOBJLIST)
	$(LINK) $^
else
$(OBJDIR)/ehslssfs.dll: $(OBJDIR)/smplsxrd.$(EXT) $(OBJDIR)/smplsdss.$(EXT) $(OBJDIR)/rmail.$(EXT) $(OBJDIR)/rmailidx.$(EXT) $(OBJDIR)/regex.$(EXT) smplssrd.def $(LIBOBJLIST)
	$(LINK) $^
endif

$(OBJDIR)/ehslscfs.dll: $(OBJDIR)/cmplsxrd.$(EXT) $(OBJDIR)/smplscrd.$(EXT) $(OBJDIR)/smplsdss.$(EXT) $(OBJDIR)/rmail.$(EXT) smplscrd.def $(LIBOBJLIST)
	$(LINK) $^

#tag: $(addsuffix .__tag,$(basename $(FILES)))

ifeq ($(COMPILER),IBM)
include $(OBJLIST:.obj=.d)
include $(LIBOBJLIST:.obj=.d)
$(OBJDIR)/%.obj: %.c
	$(CC) $(CFLAGS) $(subst /,\,$<)

$(OBJDIR)/%.obj: zlib/%.c
	$(CC) $(ZLIBCFLAGS) $(subst /,\,$<)

$(OBJDIR)/cmplsxrd.obj: smplsxrd.c
	$(CC) -DNO_RMAIL_INDEX $(CFLAGS) $(subst /,\,$<)
else
include $(OBJLIST:.obj=.d)
include $(LIBOBJLIST:.obj=.d)
$(OBJDIR)/%.obj: %.c
	$(CC) $(CFLAGS) $<

$(OBJDIR)/%.obj: zlib/%.c
	$(CC) $(ZLIBCFLAGS) $<

$(OBJDIR)/cmplsxrd.obj: smplsxrd.c
	$(CC) -DNO_RMAIL_INDEX $(CFLAGS) $<
endif

$(OBJDIR)/%.d: %.c
	@echo makedepend $<
	@if not exist $(OBJDIR) md $(OBJDIR)
	@$(MAKEDEPEND) $(INCLUDES) $(DEFINES) -I$(subst ;, -I,$(SYSINCLUDE)) $< | sed -e "s@ [a-zA-Z]:[/\\][^ ]*@@g" -e "s@^\(.*/\)*\(.*\)\.o:@$(OBJDIR)/\2.__tag $(OBJDIR)/\2.d $(OBJDIR)/\2.$(EXT): $<@" -e "s@^#.*@@g" > $@
	@echo $@: makefile >> $@

$(OBJDIR)/%.d: zlib/%.c
	@echo makedepend $<
	@if not exist $(OBJDIR) md $(OBJDIR)
	@$(MAKEDEPEND) -I$(subst ;, -I,$(SYSINCLUDE)) $< | sed -e "s@ [a-zA-Z]:[/\\][^ ]*@@g" -e "s@^\(.*/\)*\(.*\)\.o:@$(OBJDIR)/\2.__tag $(OBJDIR)/\2.d $(OBJDIR)/\2.$(EXT): $<@" -e "s@^#.*@@g" > $@
	@echo $@: makefile >> $@

$(OBJDIR)/cmplsxrd.d: smplsxrd.c
	@echo makedepend $<
	@if not exist $(OBJDIR) md $(OBJDIR)
	@$(MAKEDEPEND) -DNO_RMAIL_INDEX $(INCLUDES) $(DEFINES) -I$(subst ;, -I,$(SYSINCLUDE)) $< | sed -e "s@ [a-zA-Z]:[/\\][^ ]*@@g" -e "s@^\(.*/\)*\(.*\)\.o:@$(OBJDIR)/\2.__tag $(OBJDIR)/\2.d $(OBJDIR)/\2.$(EXT): $<@" -e "s@^#.*@@g" > $@
	@echo $@: makefile >> $@


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

install: all
	copy $(OBJDIR)\ehslscfs.dll $(INSTALL_DIR)
	copy $(OBJDIR)\ehslssfs.dll $(INSTALL_DIR)

install_boot: all
	@emxupd $(OBJDIR)\ehslscfs.dll $(INSTALL_DIR)
	@emxupd $(OBJDIR)\ehslssfs.dll $(INSTALL_DIR)
	@echo "Dll's installed. You have to reboot to use them!"

.PHONY: tags
tags:
	rm -f tags
	etags *.c *.h $(addsuffix *.c,$(VPATH)) $(addsuffix *.h,$(VPATH)) $(SM2TOOLKIT)/*.h

clean:
	rm -f *.flc *~ $(OBJS)\smplsxrd.obj $(OBJS)\smplscrd.obj $(OBJS)\smplsdss.obj $(OBJS)\ehslssfs.dll $(OBJS)\ehslscfs.dll $(OBJS)\ehslssfs.map $(OBJS)\ehslscfs.map sm2addon.zip
	rd  $(OBJS)

zip: $(OBJDIR)\ehslssfs.dll $(OBJDIR)\ehslscfs.dll readme.txt copying file_id.diz whats.new
	zip -j sm2addon $(OBJDIR)\ehslssfs.dll $(OBJDIR)\ehslscfs.dll readme.txt copying file_id.diz whats.new
