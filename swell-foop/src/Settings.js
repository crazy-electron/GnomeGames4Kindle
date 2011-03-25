Gtk = imports.gi.Gtk;
Gio = imports.gi.Gio;
GtkBuilder = imports.gtkbuilder;
main = imports.main;
ThemeLoader = imports.ThemeLoader;
GnomeGamesSupport = imports.gi.GnomeGamesSupport;
ggsconf = GnomeGamesSupport.Conf;

// Defaults
var theme, colors, zealous, size;
var default_theme = "Tango";
var default_size = 1;
var default_colors = 3;
var default_zealous = true;

// Map theme names to themes
var themes = ThemeLoader.load_themes();
var sizes = [{name: "Small", columns: 6, rows: 5},
             {name: "Normal", columns: 15, rows: 10},
             {name: "Large", columns: 20, rows: 15}];

try
{
	theme = themes[ggsconf.get_string(null, "theme")];
	size = ggsconf.get_integer(null, "size");
	colors = ggsconf.get_integer(null, "colors");
	zealous = ggsconf.get_boolean(null, "zealous");
	
	if(colors < 2 || colors > 4)
		colors = default_colors;
	
	if(theme == null)
		theme = themes[default_theme];
}
catch(e)
{
	print("Couldn't load settings from ggsconf.");
	theme = themes[default_theme];
	size = default_size;
	colors = default_colors;
	zealous = default_zealous;
}

// Settings Event Handler

SettingsWatcher = new GType({
	parent: Gtk.Button.type, // TODO: Can I make something inherit directly from GObject?!
	name: "SettingsWatcher",
	signals: [{name: "theme_changed"}, {name: "size_changed"}, {name: "colors_changed"}],
	init: function()
	{
		
	}
});

var Watcher = new SettingsWatcher();

// Settings UI

handlers = {
	select_theme: function(selector, ud)
	{
		new_theme = themes[selector.get_active_text()];

		if(new_theme == theme)
			return;
		
		theme = new_theme;
		ThemeLoader.load_theme(main.stage, theme);
		
		try
		{
			ggsconf.set_string(null, "theme", selector.get_active_text());
		}
		catch(e)
		{
			print("Couldn't save settings to ggsconf.");
		}
	
		Watcher.signal.theme_changed.emit();
	},
	set_zealous_animation: function(widget, ud)
	{
		zealous = widget.active;
		
		try
		{
			ggsconf.set_boolean(null, "zealous", zealous);
		}
		catch(e)
		{
			print("Couldn't save settings to ggsconf.");
		}
	},
	update_size: function(widget, ud)
	{
		new_size = widget.get_active();
		
		if(new_size == size)
			return;
		
		size = new_size;
		
		try
		{
			ggsconf.set_integer(null, "size", size);
		}
		catch(e)
		{
			print("Couldn't save settings to ggsconf.");
		}
		
		Watcher.signal.size_changed.emit();
	},
	update_colors: function(widget, ud)
	{
		new_colors = widget.get_value();
		
		if(new_colors == colors)
			return;

		colors = new_colors;

		try
		{
			ggsconf.set_integer(null, "colors", colors);
		}
		catch(e)
		{
			print("Couldn't save settings to ggsconf.");
		}
	
		Watcher.signal.colors_changed.emit();
	},
	reset_defaults: function(widget, ud)
	{
		print("Not yet implemented.");
	}
};

// Settings UI Helper Functions

function show_settings()
{
	b = new Gtk.Builder();
	b.add_from_file(imports.Path.file_prefix + "/settings.ui");
	b.connect_signals(handlers);

	populate_theme_selector(b.get_object("theme-selector"));
	populate_size_selector(b.get_object("size-selector"));
	
	// Set current values
	b.get_object("size-selector").set_active(size);
	b.get_object("colors-spinner").value = colors;
	b.get_object("zealous-checkbox").active = zealous;
	
	settings_dialog = b.get_object("dialog1");
	settings_dialog.set_transient_for(main.window);
	
	var result = settings_dialog.run();
	
	settings_dialog.destroy();
}

function populate_size_selector(selector)
{
	// Since we're using GtkBuilder, we can't make a Gtk.ComboBox.text. Instead,
	// we'll construct the cell renderer here, once, and use that.
	var cell = new Gtk.CellRendererText();
	selector.pack_start(cell, true);
	selector.add_attribute(cell, "text", 0);

	for(var i in sizes)
	{
		selector.append_text(sizes[i].name);
	}
}

function populate_theme_selector(selector)
{
	// Since we're using GtkBuilder, we can't make a Gtk.ComboBox.text. Instead,
	// we'll construct the cell renderer here, once, and use that.
	var cell = new Gtk.CellRendererText();
	selector.pack_start(cell, true);
	selector.add_attribute(cell, "text", 0);

	var i = 0;

	for(var th in themes)
	{
		selector.append_text(themes[th].name);
		
		if(themes[th].name == theme.name)
			selector.set_active(i);
		
		i++;
	}
}
