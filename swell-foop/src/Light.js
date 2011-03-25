Clutter = imports.gi.Clutter;
GLib = imports.gi.GLib;
main = imports.main;
Settings = imports.Settings;

Light = new GType({
	parent: Clutter.Group.type,
	name: "Light",
	init: function(self)
	{
		// Private
		var closed = false;
		var light_x, light_y;
		var state = Math.floor(Math.random() * Settings.colors);
	
		function theme_changed()
		{
			self.remove_actor(self.on);
			self.on = new Clutter.Clone({source: Settings.theme.colors[state]});
			self.on.set_size(main.tile_size, main.tile_size);	
			self.add_actor(self.on);
		}
		
		// Public
		this.visited = false;
	
		this.on = new Clutter.Clone({source: Settings.theme.colors[state]});
	
		this.get_state = function ()
		{
			return state;
		};
	
		this.animate_out = function (timeline)
		{
			this.on.animate_with_timeline(Clutter.AnimationMode.LINEAR, timeline,
			{
				height: main.tile_size * 2,
				width: main.tile_size * 2,
				x: -main.tile_size/2,
				y: -main.tile_size/2
			});
			
			this.animate_with_timeline(Clutter.AnimationMode.LINEAR, timeline,
			{
			   opacity: 0
			});
			
			timeline.signal.completed.connect(this.hide_light, this);
			
			// GLib.main_context_iteration();
		};
	
		this.animate_to = function (new_x, new_y, timeline)
		{
			var anim_mode = Settings.zealous ?
				            	Clutter.AnimationMode.EASE_OUT_BOUNCE :
				            	Clutter.AnimationMode.EASE_OUT_QUAD;
			this.animate_with_timeline(anim_mode, timeline,
			{
				x: new_x,
				y: new_y
			});
			
			// GLib.main_context_iteration();
		};
	
		this.get_closed = function ()
		{
			return closed;
		};
	
		this.close_tile = function (timeline)
		{
			closed = true;
			this.animate_out(timeline);
		};
	
		this.hide_light = function (timeline, light)
		{
			light.hide();
			
			delete on;
			
			if(light.anim)
			delete light.anim;
			
			return false;
		};
	
		this.set_light_x = function (new_x)
		{
			light_x = new_x;
		};
	
		this.set_light_y = function (new_y)
		{
			light_y = new_y;
		};
	
		this.get_light_x = function ()
		{
			return light_x;
		};
	
		this.get_light_y = function ()
		{
			return light_y;
		};
	
		// Implementation
		this.on.set_size(main.tile_size, main.tile_size);
	
		this.opacity = 180;
		this.reactive = true;
	
		this.set_anchor_point(main.tile_size / 2, main.tile_size / 2);
	
		this.add_actor(new Clutter.Rectangle({width: main.tile_size, height: main.tile_size, color: {alpha:255}}));
		this.add_actor(this.on);
	
		Settings.Watcher.signal.theme_changed.connect(theme_changed);
	}
});


