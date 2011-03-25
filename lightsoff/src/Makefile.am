lightsoffdir = $(pkgdatadir)/lightsoff

lightsoff_DATA = \
	About.js \
	Arrow.js \
	Board.js \
	Light.js \
	main.js \
	Path.js \
	Game.js \
	LED.js \
	Puzzle.js \
	Settings.js \
	ThemeLoader.js

bin_SCRIPTS = \
	lightsoff

lightsoff: lightsoff.in Makefile
	$(AM_V_GEN) $(SED) -e "s|%pkglibdir%|$(pkglibdir)|" -e "s|%pkgdatadir%|$(pkgdatadir)|" $< > $@

Path.js: Path.js.in
	$(AM_V_GEN) $(SED) -e "s|%pkgdatadir%|$(pkgdatadir)|" $< > $@

EXTRA_DIST = \
	lightsoff.in \
	About.js \
	Arrow.js \
	Board.js \
	Light.js \
	main.js \
	Path.js.in \
	Game.js \
	LED.js \
	Puzzle.js \
	Settings.js \
	ThemeLoader.js

CLEANFILES = \
	lightsoff \
	Path.js

DISTCLEANFILES = \
	lightsoff \
	Path.js

-include $(top_srcdir)/git.mk
