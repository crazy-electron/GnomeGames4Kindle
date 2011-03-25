swelldir=$(pkgdatadir)/swell-foop

swell_DATA = \
    About.js \
    Board.js \
    Light.js \
    main.js \
    Settings.js \
    Score.js \
    Path.js \
    ThemeLoader.js

Path.js: Path.js.in
	$(AM_V_GEN) $(SED) -e "s|%pkgdatadir%|$(pkgdatadir)|" $< > $@

EXTRA_DIST = \
    About.js \
    Board.js \
    Light.js \
    main.js \
    Settings.js \
    Score.js \
    Path.js.in \
    ThemeLoader.js

CLEANFILES = \
	Path.js

DISTCLEANFILES = \
	Path.js

-include $(top_srcdir)/git.mk
