#!/usr/bin/env seed

GtkClutter = imports.gi.GtkClutter;
Clutter = imports.gi.Clutter;
Gtk = imports.gi.Gtk;
GtkBuilder = imports.gtkbuilder;
GnomeGamesSupport = imports.gi.GnomeGamesSupport;

Gtk.init(Seed.argv);

try
{
	GtkClutter.init(Seed.argv.length, Seed.argv);
}
catch(e)
{
	print("Failed to initialise clutter: " + e.message);
	Seed.quit(1);
}

GnomeGamesSupport.runtime_init("lightsoff");
GnomeGamesSupport.stock_init();

Game = imports.Game;
Settings = imports.Settings;
About = imports.About;
themes = imports.ThemeLoader;

handlers = {
	show_settings: Settings.show_settings,
	show_about: About.show_about_dialog,
	show_help: function(selector, ud)
	{
		GnomeGamesSupport.help_display(window, "lightsoff", null);
	},
	reset_score: function(selector, ud)
	{
		game.reset_game();
	},
	quit: Gtk.main_quit
};

b = new Gtk.Builder();
b.add_from_file(imports.Path.file_prefix + "/lightsoff.ui");
b.connect_signals(handlers);

var window = b.get_object("game_window");
var clutter_embed = new GtkClutter.Embed();
window.signal.hide.connect(Gtk.main_quit);
b.get_object("game_vbox").pack_start(clutter_embed, true, true);

var stage = clutter_embed.get_stage();
stage.color = {alpha:255};
stage.set_use_fog(false);

stage.show_all();

themes.load_theme(stage, Settings.theme);

var game = new Game.GameView();
stage.add_actor(game);
stage.set_size(game.width, game.height);
clutter_embed.set_size_request(stage.width, stage.height);

stage.signal.key_release_event.connect(game.update_keyboard_selection);

window.show_all();

Gtk.main();

GnomeGamesSupport.runtime_shutdown();

