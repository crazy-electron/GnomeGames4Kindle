GnomeGamesSupport = imports.gi.GnomeGamesSupport;
Clutter = imports.gi.Clutter;
Pango = imports.gi.Pango;
main = imports.main;
Settings = imports.Settings;
gettext = imports.gettext;
_ = gettext.gettext;

var current_score = 0;
var final_score;
var highscores;

function set_score(score)
{
	current_score = score;
	main.score_label.label = Seed.sprintf(_("Score: %d"), current_score);
}

function calculate_score(n_lights)
{
	if (n_lights < 3)
		return 0;

	return (n_lights - 2) * (n_lights - 2);
}

function increment_score(tiles)
{
	var points_awarded = calculate_score(tiles);
	var new_score = current_score;
	
	if(Settings.zealous)
	{
		var score_text = new ScoreView();
		score_text.animate_score(points_awarded);
	}
	
	new_score += points_awarded;
	
	set_score(new_score);
}

function game_completed(won)
{
	set_score(current_score + 1000);
	
	final_score = new ScoreView();
	final_score.animate_final_score(current_score);
}

function show_scores_dialog()
{
	var highscores_dialog = new GnomeGamesSupport.ScoresDialog.c_new(
		main.window, highscores, _("Swell Foop Scores"));

	highscores_dialog.set_category_description(_("Size:"));
		
	highscores_dialog.run();
	highscores_dialog.hide();
}

function update_score_category()
{
	highscores.set_category(Settings.sizes[Settings.size].name);
}

ScoreView = new GType({
	parent: Clutter.Group.type,
	name: "Score",
	init: function()
	{
		// Private
		var label;
		
		// Public
		this.hide_score = function (timeline, score)
		{
			if(!score)
				score = this;
			
			score.hide();
			main.stage.remove_actor(score);
		};
		
		this.animate_score = function (points)
		{
			if(points <= 0)
				return;
			
			label.set_font_name("Bitstrem Vera Sans Bold 40");
			label.set_text("+" + points);
			
			main.stage.add_actor(this);
			this.show();
			
			var a = this.animate(Clutter.AnimationMode.EASE_OUT_SINE, 600,
			{
			    depth:  500,
			    opacity: 0
			});

			a.timeline.start();
			a.timeline.signal.completed.connect(this.hide_score, this);
		};
		
		this.animate_final_score = function (points)
		{
			label.set_font_name("Bitstrem Vera Sans 50");
			label.set_markup("<b>" + _("Game Over!") + "</b>\n" + Seed.sprintf(gettext.ngettext("%d point", "%d points", points), points));
			label.set_line_alignment(Pango.Alignment.CENTER);
			
			main.stage.add_actor(this);
			this.show();
			
			this.scale_x = this.scale_y = 0;
			
			var a = this.animate(Clutter.AnimationMode.EASE_OUT_ELASTIC,2000,
			{
				scale_x: 1,
				scale_y: 1,
				opacity: 255
			});
		};
		
		// Implementation
		label = new Clutter.Text();
		label.set_color({red:255, green:255, blue:255, alpha:255});
		
		this.anchor_gravity = Clutter.Gravity.CENTER;
		this.add_actor(label);
		label.show();
		
		this.x = main.stage.width / 2;
		this.y = main.stage.height / 2;
	}
});

// Initialize high scores with libgames-support

highscores = new GnomeGamesSupport.Scores.c_new("swell-foop", null,
                                                null, "board size", null, 0,
                                GnomeGamesSupport.ScoreStyle.PLAIN_DESCENDING);

highscores.add_category("Small", _("Small"));
highscores.add_category("Normal", _("Normal"));
highscores.add_category("Large", _("Large"));

update_score_category();

Settings.Watcher.signal.size_changed.connect(update_score_category);
