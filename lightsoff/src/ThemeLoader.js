Clutter = imports.gi.Clutter;
Gio = imports.gi.Gio;

function load_svg(theme, file)
{
	var tx = new Clutter.Texture({filename: imports.Path.file_prefix + "themes/"
	                                        + theme + "/" + file});
	tx.filter_quality = Clutter.TextureQuality.HIGH;
	tx.hide();
	return tx;
}

function load_theme(stage, theme)
{
	if(theme.loaded)
		return;
	
	theme.loaded = true;

	for(actor in theme.textures)
	{
	    if(theme.textures[actor].get_parent())
	        continue;
	    
		stage.add_actor(theme.textures[actor]);
		theme.textures[actor].hide();
	}
}

function load_themes()
{
	themes = {};
	
	file = Gio.file_new_for_path(imports.Path.file_prefix + "/themes");
	enumerator = file.enumerate_children("standard::name");
	
	while((child = enumerator.next_file()))
	{
		var c_theme = imports.themes[child.get_name()].theme;
		themes[c_theme.name] = c_theme;
	}
	
	return themes;
}
