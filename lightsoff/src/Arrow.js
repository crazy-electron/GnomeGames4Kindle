Settings = imports.Settings;
Clutter = imports.gi.Clutter;

ArrowView = new GType({
	parent: Clutter.Group.type,
	name: "ArrowView",
	init: function()
	{
		// Private
		
		var direction = 0;
		var arrow = new Clutter.Clone({source: Settings.theme.arrow});
		
		// Public
		
		this.flip_arrow = function()
		{
			this.rotation_angle_z = 180;
			direction = 1;
		}
		
		// Implementation
		
		this.anchor_gravity = Clutter.Gravity.CENTER;
		this.add_actor(arrow);
		this.reactive = true;
	}
});
