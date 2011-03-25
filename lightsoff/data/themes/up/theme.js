Clutter = imports.gi.Clutter;
ThemeLoader = imports.ThemeLoader;

var name = "Shine";
var theme_colorable = false;

var light = [ ThemeLoader.load_svg("up", "off.svg"),
              ThemeLoader.load_svg("up", "on.svg") ];
var arrow = ThemeLoader.load_svg("up", "arrow.svg");
var backing = ThemeLoader.load_svg("up", "backing.svg");
var led_back = ThemeLoader.load_svg("up", "led-back.svg");
var highlight = ThemeLoader.load_svg("up", "highlight.svg");

var loaded = false;
var textures = [light[0], light[1], arrow, backing, led_back, highlight];

function reload_theme()
{
    // TODO: draw with Cairo
}
