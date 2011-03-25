bin_PROGRAMS = gnect

gnect_SOURCES = main.h     \
                main.c     \
                gfx.h      \
                gfx.c      \
                prefs.h    \
                prefs.c    \
                theme.c    \
                theme.h    \
                adjmtrx.c  \
                bintree.c  \
                con4vals.h \
                connect4.h \
                connect4.c \
                evaluate.c \
                heurist.h  \
                heurist.c  \
                ia_main.c  \
                pbsolver.c \
                playgame.c \
                pnsearch.h \
                proto.h    \
                rules.h

gnect_CPPFLAGS = \
	-I$(top_srcdir) \
	$(AM_CPPFLAGS)

gnect_CFLAGS = \
	$(GTK_CFLAGS) \
	$(AM_CFLAGS)

gnect_LDADD = \
	$(top_builddir)/libgames-support/libgames-support.la \
	$(GTK_LIBS) \
	$(INTLLIBS) \
	-lz \
	$(NULL)

if HAVE_GNOME
gnect_CFLAGS += $(GNOME_CFLAGS)
gnect_LDADD += $(GNOME_LIBS)
endif

if HAVE_RSVG
gnect_CFLAGS += $(RSVG_CFLAGS)
gnect_LDADD += $(RSVG_LIBS)
endif

if WITH_GTHREAD
gnect_CFLAGS += $(GHTREAD_CFLAGS)
gnect_LDADD += $(GTHREAD_LIBS)
endif

-include $(top_srcdir)/git.mk
