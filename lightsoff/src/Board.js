Settings = imports.Settings;
GLib = imports.gi.GLib;
Clutter = imports.gi.Clutter;
Light = imports.Light;
Puzzle = imports.Puzzle;

var tiles = 5;
var puzzle_gen = new Puzzle.Gen(tiles);

// All lights need to be shifted down and right by half a light,
// as lights have center gravity.
function position_for_light(x, y)
{
	var p_l = {x: (x + 0.5) * Settings.theme.light[0].width + 2,
	           y: (y + 0.5) * Settings.theme.light[0].height + 2};
	
	return p_l;
}

BoardView = new GType({
	parent: Clutter.Group.type,
	name: "BoardView",
	signals: [{name: "game_won"}],
	class_init: function(klass, prototype)
	{
		function fade(out)
		{
			return function(timeline)
			{
				this.animate_with_timeline(Clutter.AnimationMode.EASE_OUT_SINE, 
			                               timeline,
				{
					opacity: out ? 0 : 255
				});
			}
		}
	
		prototype.fade_in = fade(false);
		prototype.fade_out = fade(true);
		
		function slide(out)
		{
			return function(direction, sign, timeline)
			{
				this.x = out ? 0 : (-sign) * direction * this.width;
				this.y = out ? 0 : (-sign) * !direction * this.height;
				
				this.animate_with_timeline(Clutter.AnimationMode.EASE_OUT_BOUNCE, 
					                       timeline,
				{
					x: out ? sign * direction * this.width : 0,
					y: out ? sign * !direction * this.height : 0
				});
			}
		}
		
		prototype.slide_in = slide(false);
		prototype.slide_out = slide(true);
		
		function swap(out)
		{
			return function(direction, timeline)
			{
				this.animate_with_timeline(Clutter.AnimationMode.EASE_IN_SINE, 
					                       timeline,
				{
					depth: out ? (250 * direction) : 0,
					x: 0,
					y: 0,
					opacity: out ? 0 : 255
				});
			}
		}
		
		prototype.swap_in = swap(false);
		prototype.swap_out = swap(true);
	},
	init: function(self)
	{
		// Private
		var playable = true;
		var lights = [];
		var loading_level = false;
		
		// Create a two-dimensional array of 'tiles*tiles' lights,
		// connect to their clicked signals, and display them.
		var create_lights = function()
		{
			for(var x = 0; x < tiles; x++)
			{
				lights[x] = [];
				
				for(var y = 0; y < tiles; y++)
				{
					var l = new Light.LightView();
					var loc = position_for_light(x, y);
					l.set_position(loc.x, loc.y);
					l.signal.button_press_event.connect(light_clicked, {"x":x, "y":y});
					
					lights[x][y] = l;
					self.add_actor(l);
					
					// GLib.main_context_iteration(null, false);
				}
			}
		}
		
		// Check if the game was won; if so, emit the game_won signal
		// in order to notify the Game controller.
		var check_won = function()
		{
			if(cleared())
				self.signal.game_won.emit();
		}
		
		// Callback for button_press_event from each light; user_data
		// is an object containing the coordinates of the clicked light.
		var light_clicked = function(light, event, coords)
		{
			self.light_toggle(coords.x, coords.y);
			
			return false;
		}
		
		// Returns whether or not the board is entirely 'off' (i.e. game is won)
		var cleared = function()
		{
			for(var x = 0; x < tiles; x++)
				for(var y = 0; y < tiles; y++)
					if(lights[x][y].get_state())
						return false;
			
			return true;
		}
		
		// Public
		
		// Toggle a light and those in each cardinal direction around it.
		this.light_toggle = function(x, y)
		{
			if(!playable)
				return;
			
			var timeline = null;
			
			if(!loading_level)
			{
				timeline = new Clutter.Timeline({duration: 300});
				timeline.signal.completed.connect(check_won);
			}
			
			if(x + 1 < tiles)
				lights[x + 1][y].toggle(timeline);
			if(x - 1 >= 0)
				lights[x - 1][y].toggle(timeline);
			if(y + 1 < tiles)
				lights[x][y + 1].toggle(timeline);
			if(y - 1 >= 0)
				lights[x][y - 1].toggle(timeline);

			lights[x][y].toggle(timeline);
			
			if(!loading_level)
				timeline.start();
		}
		
		// Pseudorandomly generates and sets the state of each light based on
		// a level number; hopefully this is stable between machines, but that
		// depends on GLib's PRNG stability. Also, provides some semblance of 
		// symmetry for some levels.
		this.load_level = function(level)
		{
			loading_level = true;
			
			// We *must* not have level < 1, as the following assumes
			// a nonzero, nonnegative number
			if(level < 1)
			{
				level = 1;
			}
			
			for(var x = 0; x < tiles; x++)
				for(var y = 0; y < tiles; y++)
					lights[x][y].set_state(0, 0);
			
			GLib.random_set_seed(level);
			
			// Determine the solution length (number of clicks required)
			// based on the level number.
			var solution_length = Math.floor(2 * Math.log(level) + 1);
			var sol = puzzle_gen.minimal_solution(solution_length);

			for(var x = 0; x < tiles; x++)
				for(var y = 0; y < tiles; y++)
					if(sol[tiles * y + x])
						self.light_toggle(x, y);
			
			loading_level = false;
		}
		
		// Set whether or not clicks on the gameboard should respond;
		// this is used to prevent clicks from registering while the board is
		// animating.
		this.set_playable = function(p)
		{
			playable = p;
		}
		
		// Implementation
		
		create_lights();
	}
});

