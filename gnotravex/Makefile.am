SUBDIRS = pixmaps

if BUILD_HELP
SUBDIRS += help
endif

NULL =

bin_PROGRAMS = gnotravex

gnotravex_SOURCES = \
	gnotravex.c \
	$(NULL)

gnotravex_CPPFLAGS = \
	-I$(top_srcdir) \
	$(AM_CPPFLAGS)

gnotravex_CFLAGS = \
	$(GTK_CFLAGS)	\
	$(AM_CFLAGS)

gnotravex_LDADD =	\
	$(top_builddir)/libgames-support/libgames-support.la \
	$(GTK_LIBS)	\
	$(INTLLIBS)	\
	$(NULL)

if HAVE_GNOME
gnotravex_CFLAGS += $(GNOME_CFLAGS)
gnotravex_LDADD += $(GNOME_LIBS)
endif

if HAVE_RSVG
gnotravex_CFLAGS += $(RSVG_CFLAGS)
gnotravex_LDADD += $(RSVG_LIBS)
endif

if WITH_GTHREAD
gnotravex_CFLAGS += $(GHTREAD_CFLAGS)
gnotravex_LDADD += $(GTHREAD_LIBS)
endif

desktop_in_files = gnotravex.desktop.in.in
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

schema_in_files = gnotravex.schemas.in
if HAVE_GNOME
schemadir = $(GCONF_SCHEMA_FILE_DIR)
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
endif

@INTLTOOL_SCHEMAS_RULE@

man_MANS = gnotravex.6

EXTRA_DIST = \
	README \
	$(schema_in_files) \
	$(man_MANS) \
	$(NULL)

CLEANFILES = $(desktop_DATA) $(schema_DATA)
DISTCLEANFILES = $(desktop_DATA) $(schema_DATA)

install-schemas-local: $(schema_DATA)
if GCONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)" ; then \
		for p in $^ ; do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $$p 2>&1 > /dev/null; \
		done \
	fi
endif

SCOREFILES = 2 3 4 5 6

install-scorefiles-local:
	-$(mkinstalldirs) $(DESTDIR)$(scoredir)
	-for i in ${SCOREFILES} ; do \
	   touch $(DESTDIR)$(scoredir)/gnotravex."$$i"x"$$i".scores; \
	   chown $(scores_user):$(scores_group) $(DESTDIR)$(scoredir)/gnotravex."$$i"x"$$i".scores; \
	   chmod 664 $(DESTDIR)$(scoredir)/gnotravex."$$i"x"$$i".scores; \
	 done

install-exec-hook:
	-if test "$(setgid)" = "true"; then \
	  chgrp $(scores_group) $(DESTDIR)$(bindir)/gnotravex && chmod 2555 $(DESTDIR)$(bindir)/gnotravex ;\
	fi

install-data-local: install-schemas-local install-scorefiles-local

-include $(top_srcdir)/git.mk
