bin_PROGRAMS = glchess

noinst_PROGRAMS = test-chess-game test-chess-pgn

TESTS = test-chess-game test-chess-pgn

glchess_SOURCES = \
	config.vapi \
	gl.vapi \
	glu.vapi \
	glx.vapi \
	3ds.vala \
	glchess.vala \
	ai-profile.vala \
	chess-bitboard.vala \
	chess-clock.vala \
	chess-engine.vala \
	chess-engine-cecp.vala \
	chess-engine-uci.vala \
	chess-game.vala \
	chess-pgn.vala \
	chess-scene.vala \
	chess-view.vala \
	chess-view-2d.vala \
	chess-view-3d.vala \
	history.vala

test_chess_game_SOURCES = \
	chess-bitboard.vala \
	chess-clock.vala \
	chess-game.vala \
	test-chess-game.vala
test_chess_game_CFLAGS = \
	$(GOBJECT_CFLAGS)
test_chess_game_LDADD = \
	$(GOBJECT_LIBS)
test_chess_game_VALAFLAGS = \
    --pkg gobject-2.0 \
    --pkg posix

test_chess_pgn_SOURCES = \
	chess-pgn.vala \
	test-chess-pgn.vala
test_chess_pgn_CFLAGS = \
	$(GIO_CFLAGS)
test_chess_pgn_LDADD = \
	$(GIO_LIBS)
test_chess_pgn_VALAFLAGS = \
    --pkg gio-2.0

glchess_CFLAGS = \
	-DPKGDATADIR=\"@datadir@/glchess\" \
	$(GMODULE_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GLX_CFLAGS) \
	$(OPENGL_CFLAGS) \
	$(RSVG_CFLAGS) \
	$(SQLITE_CFLAGS) \
	$(GTKGLEXT_CFLAGS) \
	$(WARN_CFLAGS)

glchess_LDADD = \
	$(GMODULE_LIBS) \
	$(GTK_LIBS) \
	$(GDK_X11_LIBS) \
	$(GLX_LIBS) \
	$(OPENGL_LIBS) \
	$(RSVG_LIBS) \
	$(SQLITE_LIBS) \
	$(GTKGLEXT_LIBS)

glchess_VALAFLAGS = \
    --pkg gtk+-2.0 \
    --pkg gdk-x11-2.0 \
    --pkg gmodule-2.0 \
    --pkg librsvg-2.0 \
    --pkg posix \
    --pkg sqlite3

CLEANFILES = \
	$(patsubst %.vala,%.c,$(filter %.vala, $(SOURCES))) \
	*_vala.stamp

DISTCLEANFILES = \
	Makefile.in

-include $(top_srcdir)/git.mk
