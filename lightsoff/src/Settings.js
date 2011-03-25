Gtk = imports.gi.Gtk;
Gio = imports.gi.Gio;
GtkBuilder = imports.gtkbuilder;
main = imports.main;
GConf = imports.gi.GConf;
ThemeLoader = imports.ThemeLoader;

GConf.init(Seed.argv);

// Defaults
var theme, score, use_theme_colors, theme_name;
var themes;
var default_theme_name = "Tango";
var default_use_theme_colors = false;

// Load settings from GConf
try
{
	gconf_client = GConf.Client.get_default();
	score = gconf_client.get_int("/apps/lightsoff/score");
	theme_name = gconf_client.get_string("/apps/lightsoff/theme")
	use_theme_colors = gconf_client.get_bool("/apps/lightsoff/use_theme_colors");
	
	if(theme == null)
		theme = default_theme_name;
}
catch(e)
{
	print("Couldn't load settings from GConf.");
	theme_name = default_theme_name;
	score = 1;
	use_theme_colors = default_use_theme_colors;
}

// If (machine isn't restarted?) after schema is installed, defaults
// from GConf aren't set, so revert to defaults...
// TODO: this seems unacceptable and wrong
if(score == 0)
{
	theme_name = default_theme_name;
	score = 1;
}

try
{
    // Map theme names to themes
    themes = ThemeLoader.load_themes();
    
    // Load selected theme
    theme = themes[theme_name];
}
catch(e)
{
    print("Couldn't load selected theme...");
}

// Settings Event Handler

SettingsWatcher = new GType({
	parent: Gtk.Button.type,
	name: "SettingsWatcher",
	signals: [{name: "theme_changed"}],
	init: function()
	{
		
	}
});

var Watcher = new SettingsWatcher();

// Watch for Gtk theme change, reload theme!

function reload_on_theme_change(widget, last_style, user_data)
{
    if(main.game.is_animating())
        return;

    theme.reload_theme();
    ThemeLoader.load_theme(main.stage, theme);
    Watcher.signal.theme_changed.emit();
}

var fakeWindow = new Gtk.Window();
fakeWindow.realize();
fakeWindow.signal.style_set.connect(reload_on_theme_change);

// Settings UI

handlers = {
	select_theme: function(selector, ud)
	{
		new_theme = themes[selector.get_active_text()];
		
		if(new_theme == theme)
			return;
		
		theme = new_theme;
		ThemeLoader.load_theme(main.stage, theme);
		
		b.get_object("use-gnome-theme-checkbox").sensitive = theme.theme_colorable;
		
		if(!theme.theme_colorable)
		    b.get_object("use-gnome-theme-checkbox").active = false;
		
		try
		{
			gconf_client.set_string("/apps/lightsoff/theme", selector.get_active_text());
		}
		catch(e)
		{
			print("Couldn't save settings to GConf.");
		}
	
		Watcher.signal.theme_changed.emit();
	},
	set_use_theme_colors: function(widget, ud)
	{
	    use_theme_colors = widget.active;
	    
	    theme.reload_theme();
	    ThemeLoader.load_theme(main.stage, theme);
	    
	    try
	    {
	        gconf_client.set_bool("/apps/lightsoff/use_theme_colors", use_theme_colors);
        }
        catch(e)
        {
            print("Couldn't save settings to GConf.");
        }
        
        Watcher.signal.theme_changed.emit();
	}
};

// Settings UI Helper Functions

function show_settings()
{
	b = new Gtk.Builder();
	b.add_from_file(imports.Path.file_prefix + "/settings.ui");
	b.connect_signals(handlers);

	populate_theme_selector(b.get_object("theme-selector"));

    // Set current values
    b.get_object("use-gnome-theme-checkbox").active = use_theme_colors;
    b.get_object("use-gnome-theme-checkbox").sensitive = theme.theme_colorable;

	settings_dialog = b.get_object("dialog1");
	settings_dialog.set_transient_for(main.window);
	
	var result = settings_dialog.run();
	
	settings_dialog.destroy();
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
