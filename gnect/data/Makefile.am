NULL =

velenadir      = $(pkgdatadir)/gnect
velena_DATA    = white_ob.cn4.gz

Games_in_files = gnect.desktop.in.in
Gamesdir       = $(datadir)/applications
Games_DATA     = $(Games_in_files:.desktop.in.in=.desktop)

schema_in_files = gnect.schemas.in
if HAVE_GNOME
schemadir = $(GCONF_SCHEMA_FILE_DIR)
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
endif

man_MANS     = gnect.6

EXTRA_DIST = $(velena_DATA) \
             $(schema_in_files) \
             $(man_MANS)


CLEANFILES = $(NULL)

DISTCLEANFILES = $(schema_DATA) $(Games_DATA)

install-data-local: 
if GCONF_SCHEMAS_INSTALL
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/gnect/data/$(schema_DATA)
endif

@INTLTOOL_DESKTOP_RULE@
@INTLTOOL_SCHEMAS_RULE@

-include $(top_srcdir)/git.mk
