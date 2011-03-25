Settings = imports.Settings;
GLib = imports.gi.GLib;
Clutter = imports.gi.Clutter;
LED = imports.LED;
Board = imports.Board;
Arrow = imports.Arrow;
main = imports.main;

var last_direction, last_sign;

GameView = new GType({
	parent: Clutter.Group.type,
	name: "GameView",
	init: function()
	{
		// Private
		var self = this;
		var current_level = 1;
		var queue_theme_change = 0;
		var actor_remove_queue = [];
		var score_view = new LED.LEDView();
		var board_view = new Board.BoardView();
		var backing_view = new Clutter.Clone({source:Settings.theme.backing});
		var left_arrow = new Arrow.ArrowView();
		var right_arrow = new Arrow.ArrowView();
		var keycursor_view = new Clutter.Clone({source:Settings.theme.highlight});
		var new_board_view = null;
		var timeline;
		var keycursor = {x:0, y:0, ready: false};
		
		// Set up a new board.
		var create_next_board = function()
		{
			new_board_view = new Board.BoardView();
			new_board_view.load_level(current_level);
			new_board_view.signal.game_won.connect(game_won);
			new_board_view.hide();
			new_board_view.set_playable(false);
			self.add_actor(new_board_view);
			new_board_view.lower_bottom();
			backing_view.raise_top();
			score_view.raise_top();
			left_arrow.raise_top();
			right_arrow.raise_top();
		}
		
		// The boards have finished transitioning; delete the old one!
		var board_transition_complete = function()
		{
			self.remove_actor(board_view);
			board_view = new_board_view;
			board_view.set_playable(true);
			keycursor_view.raise_top();
			new_board_view = timeline = null;

			if(queue_theme_change > 0)
				theme_changed();
			
			queue_theme_change = 0;
			
			// Remove all of the queued-for-removal actors
			while(actor_remove_queue.length > 0)
			{
				act = actor_remove_queue.pop();

				if(act && act.get_parent())
					act.get_parent().remove_actor(act);
			}
		}
		
		// The player won the game; create a new board, update the level count,
		// and transition between the two boards in a random direction.
		var game_won = function()
		{
			if(timeline && timeline.is_playing())
				return false;
			
			var direction, sign;
			
			score_changed(++current_level);
			
			// Make sure the board transition is different than the previous.
			do
			{
				direction = Math.floor(2 * Math.random());
				sign = Math.floor(2 * Math.random()) ? 1 : -1;
			}
			while(last_direction == direction || last_sign == sign);
				
			last_direction = direction;
			last_sign = sign;
			
			timeline = new Clutter.Timeline({duration: 1500});
			
			create_next_board();
			new_board_view.show();
			
			new_board_view.slide_in(direction, sign, timeline);
			board_view.slide_out(direction, sign, timeline);
			timeline.signal.completed.connect(board_transition_complete);
				
			return false;
		}
		
		// The player asked to swap to a different level without completing
		// the one in progress; this can occur either by clicking an arrow
		// or by requesting a new game from the menu. Animate the new board
		// in, depthwise, in the direction indicated by 'context'.
		var swap_board = function(arrow, event, context)
		{
			if(timeline && timeline.is_playing())
				return false;
			
			current_level += context.direction;
			
			if(current_level <= 0)
			{
				current_level = 1;
				return false;
			}
			
			score_changed(current_level);
			
			timeline = new Clutter.Timeline({duration: 500});
			
			create_next_board();
			
			new_board_view.depth = context.direction * -250;
			new_board_view.opacity = 0;
			
			new_board_view.show();
			
			new_board_view.swap_in(context.direction, timeline);
			board_view.swap_out(context.direction, timeline);
			timeline.signal.completed.connect(board_transition_complete);
			
			timeline.start();
			
			return false;
		}
		
		// The player changed the theme from within the preferences window
		var theme_changed = function()
		{
			if(timeline)
			{
				queue_theme_change++;
				return;
			}
			
			timeline = new Clutter.Timeline({duration: 1500});
			
			create_next_board();
			new_board_view.opacity = 0;
			new_board_view.show();
			
			board_view.fade_out(timeline);
			new_board_view.fade_in(timeline);
			new_board_view.raise_top();
			
			timeline.signal.completed.connect(board_transition_complete);
			timeline.start();
		}
		
		// The current level changed; update the score in GConf and the LEDView 
		var score_changed = function(new_score)
		{
			current_level = new_score;
			
			try
			{
				Settings.gconf_client.set_int("/apps/lightsoff/score",
											  current_level);
			}
			catch(e)
			{
				print("Couldn't save score to GConf.");
			}
			
			score_view.set_value(current_level);
		}
		
		// Public
		
		this.is_animating = function ()
		{
			return (timeline != null);
		}
		
		// Queue an actor to be removed after the board is finished reloading
		this.queue_actor_remove = function (actor)
		{
			actor_remove_queue.push(actor);
		}
		
		this.reset_game = function ()
		{
			if(timeline && timeline.is_playing())
				return false;
			
			score_changed(1);
			
			timeline = new Clutter.Timeline({duration: 500});
			
			create_next_board();
			
			new_board_view.depth = 250;
			new_board_view.opacity = 0;
			
			new_board_view.show();
			
			new_board_view.swap_in(-1, timeline);
			board_view.swap_out(-1, timeline);
			timeline.signal.completed.connect(board_transition_complete);
			
			timeline.start();
			
			return false;
		}
		
		// Change the currently selected tile with the keyboard
		this.update_keyboard_selection = function (actor, event, ud)
		{
			if(event.key.keyval == Clutter.Escape)
			{
				keycursor_view.animate(Clutter.AnimationMode.EASE_OUT_SINE, 250,
				{
					opacity: 0
				});
				
				keycursor.ready = false;
			}
			
			keycursor_view.raise_top();
			
			if(keycursor.ready)
			{
				if(event.key.keyval == Clutter.Up && keycursor.y > 0)
					keycursor.y--;
				else if(event.key.keyval == Clutter.Down && keycursor.y < 4)
					keycursor.y++;
				else if(event.key.keyval == Clutter.Left && keycursor.x > 0)
					keycursor.x--;
				else if(event.key.keyval == Clutter.Right && keycursor.x < 4)
					keycursor.x++;
				else if(event.key.keyval == Clutter.Return)
					board_view.light_toggle(keycursor.x, keycursor.y);
			}
			
			if(event.key.keyval != Clutter.Down &&
			   event.key.keyval != Clutter.Up &&
			   event.key.keyval != Clutter.Left &&
			   event.key.keyval != Clutter.Right)
				return false;
		
			var loc = Board.position_for_light(keycursor.x, keycursor.y);
			
			if(keycursor.ready)
			{
				keycursor_view.animate(Clutter.AnimationMode.EASE_OUT_SINE, 250,
				{
					x: loc.x,
					y: loc.y
				});
			}
			else
			{
				keycursor_view.opacity = 0;
				keycursor_view.set_position(loc.x, loc.y);
				
				keycursor_view.animate(Clutter.AnimationMode.EASE_OUT_SINE, 250,
				{
					opacity: 255
				});
			}
			
			keycursor.ready = true;
	
			return false;
		}
		
		// Implementation
		
		board_view.show_all();
		var real_board_width = 5 * Settings.theme.light[0].width + 4;
		var real_board_height = 5 * Settings.theme.light[0].height + 4;
		
		score_view.set_width(5);
		score_changed(Settings.score);
		
		backing_view.set_position(0, real_board_height);
		this.add_actor(backing_view);
		
		score_view.set_anchor_point(score_view.width / 2, 0);
		score_view.set_position(real_board_width / 2, real_board_height + 18);
		this.add_actor(score_view);
		
		// TODO: The -10 term in the next two Y locations makes me sad.
		
		left_arrow.set_position((score_view.x - score_view.anchor_x) / 2,
								score_view.y + (score_view.height / 2) - 10);
		this.add_actor(left_arrow);
		
		right_arrow.flip_arrow();
		right_arrow.set_position(real_board_width - left_arrow.x,
								 score_view.y + (score_view.height / 2) - 10);
		this.add_actor(right_arrow);
		
		left_arrow.signal.button_release_event.connect(swap_board, {direction: -1});
		right_arrow.signal.button_release_event.connect(swap_board, {direction: 1});
		
		this.set_size(real_board_width, score_view.y + score_view.height);
		
		keycursor_view.set_position(-100, -100);
		keycursor_view.anchor_gravity = Clutter.Gravity.CENTER;
		this.add_actor(keycursor_view);

		Settings.Watcher.signal.theme_changed.connect(theme_changed);
		
		// Set up and show the initial board
		board_view.signal.game_won.connect(game_won);
		board_view.load_level(current_level);
		this.add_actor(board_view);
		create_next_board();
	}
});

