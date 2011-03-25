#!/usr/bin/env seed

var tile_size = 50;
var offset = tile_size/2;

imports.gi.versions.Clutter = "1.0";

Gtk = imports.gi.Gtk;
GtkClutter = imports.gi.GtkClutter;
GtkBuilder = imports.gtkbuilder;
Clutter = imports.gi.Clutter;
GnomeGamesSupport = imports.gi.GnomeGamesSupport;
_ = imports.gettext.gettext;

try
{
	GtkClutter.init(Seed.argv.length, Seed.argv);
}
catch(e)
{
	print("Failed to initialise clutter: " + e.message);
	Seed.quit(1);
}

if(GnomeGamesSupport.setgid_io_init)
	GnomeGamesSupport.setgid_io_init();

GnomeGamesSupport.runtime_init("swell-foop");
GnomeGamesSupport.Conf.initialise("swell-foop");
GnomeGamesSupport.stock_init();

Light = imports.Light;
Board = imports.Board;
Score = imports.Score;
About = imports.About;
Settings = imports.Settings;
ThemeLoader = imports.ThemeLoader;

handlers = {
	show_settings: function(selector, ud)
	{
		Settings.show_settings();
	},
	show_about: function(selector, ud)
	{
		About.show_about_dialog();
	},
	show_scores: function(selector, ud)
	{
		Score.show_scores_dialog();
	},
	show_help: function(selector, ud)
	{
		GnomeGamesSupport.help_display(window, "swell-foop", null);
	},
	new_game: function(selector, ud)
	{
		board.new_game();
	},
	quit: function(selector, ud)
	{
		Gtk.main_quit();
	}
};

size_o = Settings.sizes[Settings.size];

b = new Gtk.Builder();
b.add_from_file(imports.Path.file_prefix + "/swell-foop.ui");
b.connect_signals(handlers);

var window = b.get_object("game_window");
var clutter_embed = b.get_object("clutter");
var message_label = b.get_object("message_label");
var score_label = b.get_object("score_label");

var stage = clutter_embed.get_stage();

stage.signal.hide.connect(Gtk.main_quit);
stage.set_use_fog(false);

stage.color = {alpha: 0};
stage.set_size((size_o.columns * tile_size),
               (size_o.rows * tile_size));
clutter_embed.set_size_request((size_o.columns * tile_size),
                               (size_o.rows * tile_size));

// NOTE: show the window before the stage, and the stage before any children
window.show_all();
stage.show_all();

ThemeLoader.load_theme(stage, Settings.theme);

function size_changed()
{
	size_o = Settings.sizes[Settings.size];
	
	stage.set_size((size_o.columns * tile_size),
	               (size_o.rows * tile_size));
	clutter_embed.set_size_request((size_o.columns * tile_size),
	                               (size_o.rows * tile_size));

	var new_board = new Board.Board();
	new_board.new_game();
	stage.add_actor(new_board);
	stage.remove_actor(board);
	board.show();
	board = new_board;
}

Settings.Watcher.signal.size_changed.connect(size_changed);

var board = new Board.Board();
stage.add_actor(board);
board.show();

board.new_game();

Gtk.main();

GnomeGamesSupport.Conf.shutdown();
GnomeGamesSupport.runtime_shutdown();

