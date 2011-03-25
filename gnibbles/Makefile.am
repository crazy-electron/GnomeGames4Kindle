SUBDIRS = pix

if BUILD_HELP
SUBDIRS += help
endif

NULL =

bin_PROGRAMS = gnibbles

gnibbles_SOURCES = \
	bonus.h \
	bonus.c \
	boni.h \
	boni.c \
	warp.h \
	warp.c \
	warpmanager.h \
	warpmanager.c \
	gnibbles.h \
	gnibbles.c \
	properties.h \
	properties.c \
	preferences.h \
	preferences.c \
	scoreboard.h \
	scoreboard.c \
	main.c \
	main.h \
	board.c \
	board.h \
	worm.c \
	worm.h \
	$(NULL)

gnibbles_CPPFLAGS = \
	-I$(top_srcdir) \
	$(AM_CPPFLAGS)

gnibbles_CFLAGS = \
	$(GTK_CFLAGS) \
	$(CLUTTER_GTK_CFLAGS) \
	$(CLUTTER_CFLAGS) \
	$(AM_CFLAGS)

gnibbles_LDADD = \
	$(top_builddir)/libgames-support/libgames-support.la \
	$(GTK_LIBS) \
	$(CLUTTER_GTK_LIBS) \
	$(CLUTTER_LIBS) \
	$(INTLLIBS)

if HAVE_GNOME
gnibbles_CFLAGS += $(GNOME_CFLAGS)
gnibbles_LDADD += $(GNOME_LIBS)
endif

if HAVE_RSVG
gnibbles_CFLAGS += $(RSVG_CFLAGS)
gnibbles_LDADD += $(RSVG_LIBS)
endif

if WITH_GTHREAD
gnibbles_CFLAGS += $(GHTREAD_CFLAGS)
gnibbles_LDADD += $(GTHREAD_LIBS)
endif

pixmapdir = $(datadir)/gnome-games/gnibbles/pixmaps
pixmap_DATA = gnibbles-logo.svg

configdir = $(datadir)/gnome-games/gnibbles/games
config_DATA = \
	level001.gnl \
	level002.gnl \
	level003.gnl \
	level004.gnl \
	level005.gnl \
	level006.gnl \
	level007.gnl \
	level008.gnl \
	level009.gnl \
	level010.gnl \
	level011.gnl \
	level012.gnl \
	level013.gnl \
	level014.gnl \
	level015.gnl \
	level016.gnl \
	level017.gnl \
	level018.gnl \
	level019.gnl \
	level020.gnl \
	level021.gnl \
	level022.gnl \
	level023.gnl \
	level024.gnl \
	level025.gnl \
	level026.gnl \
	$(NULL)

desktopdir=$(datadir)/applications
desktop_in_files = gnibbles.desktop.in.in
desktop_DATA = $(desktop_in_files:.desktop.in.in=.desktop)

schema_in_files = gnibbles.schemas.in
if HAVE_GNOME
schemadir = $(GCONF_SCHEMA_FILE_DIR)
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
endif

man_MANS = gnibbles.6

EXTRA_DIST = \
	$(pixmap_DATA) 	\
	$(config_DATA) \
	$(schema_in_files) \
	$(man_MANS)

CLEANFILES = \
	$(NULL)

DISTCLEANFILES = $(desktop_DATA) $(schema_DATA)

SCOREFILES = 4.0 3.0 2.0 1.0 4.1 3.1 2.1 1.1

install-data-local:
if GCONF_SCHEMAS_INSTALL
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/gnibbles/$(schema_DATA)
endif
	-$(mkinstalldirs) $(DESTDIR)$(scoredir)
	-for i in ${SCOREFILES} ; do \
	   touch $(DESTDIR)$(scoredir)/gnibbles.$$i.scores; \
	   chown $(scores_user):$(scores_group) $(DESTDIR)$(scoredir)/gnibbles.$$i.scores; \
	   chmod 664 $(DESTDIR)$(scoredir)/gnibbles.$$i.scores; \
	 done

install-exec-hook:
	-if test "$(setgid)" = "true"; then \
	  chgrp $(scores_group) $(DESTDIR)$(bindir)/gnibbles && chmod 2555 $(DESTDIR)$(bindir)/gnibbles ;\
	fi

@INTLTOOL_DESKTOP_RULE@
@INTLTOOL_SCHEMAS_RULE@

-include $(top_srcdir)/git.mk
