cairo = imports.cairo;
Clutter = imports.gi.Clutter;
ThemeLoader = imports.ThemeLoader;
Gtk = imports.gi.Gtk;
Settings = imports.Settings;
main = imports.main;

var name = "Tango";
var theme_colorable = true;
var tile_size = 75;

// TODO: catch GNOME-wide theme change, update tiles, recache them, etc.

function rounded_rectangle(cr,x,y,w,h,r)
{
    cr.new_path();
	cr.move_to(x+r,y);
    cr.line_to(x+w-r,y);
    cr.curve_to(x+w,y,x+w,y,x+w,y+r);
    cr.line_to(x+w,y+h-r);
    cr.curve_to(x+w,y+h,x+w,y+h,x+w-r,y+h);
    cr.line_to(x+r,y+h);
    cr.curve_to(x,y+h,x,y+h,x,y+h-r);
    cr.line_to(x,y+r);
    cr.curve_to(x,y,x,y,x+r,y);
	cr.close_path();
}

function draw_tile(color)
{
	var tile = new Clutter.CairoTexture();
	tile.set_surface_size(tile_size, tile_size);
	var context = tile.create();
	var cr = new cairo.Context.steal(context);
	
	var pattern = new cairo.LinearGradient(tile_size,0,0,0);
	pattern.add_color_stop_rgba(0,
	                            color.red / 255,
	                            color.green / 255,
	                            color.blue / 255,
	                            color.alpha / 255);
	pattern.add_color_stop_rgba(1,
	                            color.red / 255 - 0.15,
	                            color.green / 255 - 0.15,
	                            color.blue / 255 - 0.15,
	                            color.alpha / 255);
	cr.set_source(pattern);
	
	rounded_rectangle(cr, 2, 2, tile_size - 4, tile_size - 4, 5);
	cr.fill();
	
	cr.set_source_rgba(color.red / 255 + 0.05, color.green / 255 + 0.05, color.blue / 255 + 0.05, 1.0);
	rounded_rectangle(cr, 2, 2, tile_size - 4, tile_size - 4, 5);
	cr.line_width = 2.0;
	cr.stroke();

	cr.destroy();
	
	return tile;
}

function draw_glow(color)
{
	var tile = new Clutter.CairoTexture();
	tile.set_surface_size(tile_size, tile_size);
	var context = tile.create();
	var cr = new cairo.Context.steal(context);
	
	var pattern = new cairo.LinearGradient(0,0,0,tile_size);
	pattern.add_color_stop_rgba(0, 1.0, 1.0, 1.0, 0.3);
	pattern.add_color_stop_rgba(1, 1.0, 1.0, 1.0, 0.1);
	cr.set_source(pattern);
	
	cr.new_path();
	cr.move_to(0, 0);
	cr.line_to(0, tile_size);
	cr.line_to(tile_size, tile_size);
	cr.line_to(tile_size, 0);
	cr.close_path();
	cr.fill();
	
	cr.set_source_rgba(1.0,1.0,1.0,0.4);
	
	cr.new_path();
	cr.move_to(3, 3);
	cr.line_to(3, tile_size - 3);
	cr.line_to(tile_size - 3, tile_size - 3);
	cr.line_to(tile_size - 3, 3);
	cr.close_path();
	cr.stroke();
	
	cr.destroy();
	
	return tile;
}

function reload_theme()
{
    // TODO: there must be a better way to get the Gtk selection color
    // FIXME: this is now broken, with Gtk3
    //var gtk_settings = Gtk.Settings.get_default();
    //var gtk_color_scheme = gtk_settings.gtk_color_scheme;
    //var c = new Clutter.Color();
    //c.from_string(gtk_color_scheme.match(/selected_bg_color: (.*);?/)[1]);
    
    // Remove the previous theme's cached lights
    if(light.length > 0)
    {
        main.game.queue_actor_remove(light[0]);
        main.game.queue_actor_remove(light[1]);
    }

    /*if(Settings.use_theme_colors)
    {
        light = [ draw_tile({red: 32, green: 32, blue: 32, alpha: 255}),
                  draw_tile(c) ];
    }
    else
    {*/
    light = [ ThemeLoader.load_svg("tango", "off.svg"),
              ThemeLoader.load_svg("tango", "on.svg") ];
    //}
    
    textures = [light[0], light[1], arrow, backing, led_back, highlight];
    loaded = false;
}

var light = [];
var arrow = ThemeLoader.load_svg("tango", "arrow.svg");
var backing = ThemeLoader.load_svg("tango", "backing.svg");
var led_back = ThemeLoader.load_svg("tango", "led-back.svg");
var highlight = ThemeLoader.load_svg("tango", "highlight.svg");
var loaded = false;
var textures = [];

reload_theme();
