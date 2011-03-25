ThemeLoader = imports.ThemeLoader;

var name = "Test Theme";

var colors = [ ThemeLoader.load_svg("test", "blue.svg"),
               ThemeLoader.load_svg("test", "green.svg"),
               ThemeLoader.load_svg("test", "red.svg"),
               ThemeLoader.load_svg("test", "yellow.svg") ];

var loaded = false;
var textures = [colors[0], colors[1], colors[2], colors[3]];

