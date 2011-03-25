SUBDIRS = pix

if BUILD_HELP
SUBDIRS += help
endif

bin_PROGRAMS = gtali

gtali_SOURCES =       \
	gyahtzee.c       \
	gyahtzee.h       \
	clist.c          \
	yahtzee.h        \
	yahtzee.c        \
	computer.c       \
	setup.c          \
	$(NULL)

gtali_CPPFLAGS = \
	-I$(top_srcdir) \
	$(AM_CPPFLAGS)

gtali_CFLAGS = \
	$(GTK_CFLAGS)	\
	$(AM_CFLAGS)
		
gtali_LDADD = \
	$(top_builddir)/libgames-support/libgames-support.la \
	$(GTK_LIBS)	\
	$(INTLLIBS)	\
	$(NULL)

if HAVE_GNOME
gtali_CFLAGS += $(GNOME_CFLAGS)
gtali_LDADD += $(GNOME_LIBS)
endif

if HAVE_RSVG
gtali_CFLAGS += $(RSVG_CFLAGS)
gtali_LDADD += $(RSVG_LIBS)
endif

if WITH_GTHREAD
gtali_CFLAGS += $(GHTREAD_CFLAGS)
gtali_LDADD += $(GTHREAD_LIBS)
endif

schema_in_files = gtali.schemas.in
if HAVE_GNOME
schemadir = $(GCONF_SCHEMA_FILE_DIR)
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
endif

@INTLTOOL_SCHEMAS_RULE@

man_MANS = gtali.6

EXTRA_DIST = README AUTHORS $(schema_in_files) $(man_MANS)

Gamesdir = $(datadir)/applications
Games_in_files = gtali.desktop.in.in
Games_DATA = $(Games_in_files:.desktop.in.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

install-data-local:
if GCONF_SCHEMAS_INSTALL
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/gtali/$(schema_DATA)
endif
	-$(mkinstalldirs) $(DESTDIR)$(scoredir)
	-if test -e $(DESTDIR)$(scoredir)/gtali.scores ; then mv $(DESTDIR)$(scoredir)/gtali.scores $(DESTDIR)$(scoredir)/gtali.Regular.scores ; fi
	-touch $(DESTDIR)$(scoredir)/gtali.Regular.scores
	-chown $(scores_user):$(scores_group) $(DESTDIR)$(scoredir)/gtali.Regular.scores
	-chmod 664 $(DESTDIR)$(scoredir)/gtali.Regular.scores	
	-touch $(DESTDIR)$(scoredir)/gtali.Colors.scores
	-chown $(scores_user):$(scores_group) $(DESTDIR)$(scoredir)/gtali.Colors.scores
	-chmod 664 $(DESTDIR)$(scoredir)/gtali.Colors.scores

install-exec-hook:
	-if test "$(setgid)" = "true"; then \
	  chgrp $(scores_group) $(DESTDIR)$(bindir)/gtali && chmod 2555 $(DESTDIR)$(bindir)/gtali ;\
	fi

DISTCLEANFILES = $(Games_DATA) $(schema_DATA)

-include $(top_srcdir)/git.mk
