ThemeLoader = imports.ThemeLoader;

var name = "Tango";

var colors = [ ThemeLoader.load_svg("tango", "blue.svg"),
               ThemeLoader.load_svg("tango", "green.svg"),
               ThemeLoader.load_svg("tango", "red.svg"),
               ThemeLoader.load_svg("tango", "yellow.svg") ];

var loaded = false;
var textures = [colors[0], colors[1], colors[2], colors[3]];

