Settings = imports.Settings;
Clutter = imports.gi.Clutter;

LightView = new GType({
	parent: Clutter.Group.type,
	name: "LightView",
	class_init: function(klass, prototype)
	{
		prototype.toggle = function(timeline)
		{
			this.set_state(!this.get_state(), timeline);
		};
	},
	init: function(self)
	{
		// Private
		var on = new Clutter.Clone({source: Settings.theme.light[1],
		                            anchor_gravity: Clutter.Gravity.CENTER});
		var off = new Clutter.Clone({source: Settings.theme.light[0],
		                             anchor_gravity: Clutter.Gravity.CENTER});
		var state = false;
		
		// Public
		
		// Animates to the requested lit state with the given timeline.
		this.set_state = function(new_state, timeline)
		{
			state = new_state;
			
			if(timeline)
			{
				// Animate the opacity of the 'off' tile to match the state.
				off.animate_with_timeline(Clutter.AnimationMode.EASE_OUT_SINE, timeline,
				{
					opacity: (state ? 0 : 255)
				});
			
				on.animate_with_timeline(Clutter.AnimationMode.EASE_OUT_SINE, timeline,
				{
					opacity: (state ? 255 : 0)
				});
			
				// Animate the tile to be smaller when in the 'off' state.
				self.animate_with_timeline(Clutter.AnimationMode.EASE_OUT_SINE, timeline,
				{
					scale_x: (state ? 1 : 0.9),
					scale_y: (state ? 1 : 0.9)
				});
			}
			else
			{
				off.opacity = (state ? 0 : 255);
				on.opacity = (state ? 255 : 0);
				self.scale_x = (state ? 1 : 0.9);
				self.scale_y = (state ? 1 : 0.9);
			}
		}
		
		this.get_state = function()
		{
			return state;
		}
		
		// Implementation
		
		this.add_actor(on);
		this.add_actor(off);
		
		off.raise_top();
		
		this.set_scale(0.9, 0.9);
		this.reactive = true;
		this.anchor_gravity = Clutter.Gravity.CENTER;
		
		// Add a 2 px margin around the tile image, center tiles within it.
		this.width += 4;
		this.height += 4;
		
		on.set_position(this.width / 2, this.height / 2);
		off.set_position(this.width / 2, this.height / 2);
		
		on.opacity = 0;
	}
});
