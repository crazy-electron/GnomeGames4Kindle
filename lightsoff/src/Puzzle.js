// Puzzle generation logic:
// We want to measure a puzzle's difficulty by the number of button presses
// needed to solve it, and have that increase in a controlled manner.
//
// Lights Off can be seen as a linear algebra problem over the field GF(2).
// (See e.g. the first page of "Turning Lights Out with Linear Algebra".)
// The linear map from button-press strategies to light configurations
// will in general have a nullspace (of dimension n).
// Thus, any solvable puzzle has 2^n solutions, given by a fixed solution
// plus an element of the nullspace.
// A solution is thus optimal if it presses at most half the buttons
// which make up any element of the nullspace.
//
// A basis for the nullspace splits the board into (up to) 2^n regions,
// defined by which basic nullspace elements include a given button.
// (This determines which nullspace elements include the same button.)
// Thus, a solution is optimal if it includes at most half the buttons in any
// region (except the region lying outside all null-sets).
// The converse is not true, but (at least if the regions are even-sized)
// does hold for a large number of puzzles up to the highest difficulty level.
// (Certainly, on average, at most half the lights which belong to at least
// one null set may be used.)
GLib = imports.gi.GLib;

function Gen(tiles)
{
	var i, j, ipiv, jpiv;
	var adj_matrix = [];
	for(i = 0; i < tiles*tiles; i++)
	{
		adj_matrix[i] = [];
		for(j = 0; j < tiles*tiles; j++)
		{
			var dx =           (i % tiles) -           (j % tiles);
			var dy = Math.floor(i / tiles) - Math.floor(j / tiles);
			adj_matrix[i][j] = (dx*dx + dy*dy <= 1 ? 1 : 0);
		}
	}
	// Row-reduction over field with two elements
	var non_pivot_cols = [];
	for(ipiv = jpiv = 0; jpiv < tiles*tiles; jpiv++)
	{
		var is_pivot_col = false;
		for(i = ipiv; i < tiles*tiles; i++)
		{
			if(adj_matrix[i][jpiv] != 0)
			{
				if(i != ipiv)
				{
					var swap = adj_matrix[i];
					adj_matrix[i] = adj_matrix[ipiv];
					adj_matrix[ipiv] = swap;
				}
				for(i = ipiv+1; i < tiles*tiles; i++)
				{
					if(adj_matrix[i][jpiv] != 0)
					{
						for(j = 0; j < tiles*tiles; j++)
							adj_matrix[i][j] ^= adj_matrix[ipiv][j];
					}
				}
				is_pivot_col = true;
				ipiv++;
				break;
			}
		}
		if(!is_pivot_col)
			non_pivot_cols.push(jpiv);
	}
	// Use back-substitution to solve Adj*x = 0, once with each
	// free variable set to 1 (and the others to 0).
	var basis_for_ns = [];
	non_pivot_cols.forEach(function(col)
		{
			var null_vec = [];
			for(j = 0; j < tiles*tiles; j++)
				null_vec[j] = 0;
			null_vec[col] = 1;
			for(i = tiles*tiles - 1; i >= 0; i--)
			{
				for(jpiv = 0; jpiv < tiles*tiles; jpiv++)
					if(adj_matrix[i][jpiv] != 0)
						break;
				if(jpiv == tiles*tiles)
					continue;
				for(j = jpiv + 1; j < tiles*tiles; j++)
					null_vec[jpiv] ^= adj_matrix[i][j] * null_vec[j];
			}
			basis_for_ns.push(null_vec);
		});
	// A button's region # is a binary # with 1's in a place corresponding
	// to any null-vector which contains it.
	this.region_of = [];
	this.region_size = [];
	for(j = 0; j < (1 << basis_for_ns.length); j++)
		this.region_size[j] = 0;
	for(i = 0; i < tiles*tiles; i++)
	{
		this.region_of[i] = 0;
		for(j = 0; j < basis_for_ns.length; j++)
			if(basis_for_ns[j][i] != 0)
				this.region_of[i] += (1 << j);
		this.region_size[ this.region_of[i] ]++;
	}
	this.tiles = tiles;
	this.max_solution_length = this.region_size[0];
	for(j = 1; j < this.region_size.length; j++)
		this.max_solution_length += Math.floor(this.region_size[j] / 2);
}

Gen.prototype =
{
	minimal_solution : function(solution_length)
	{
		var sol = [];
		for(var i = 0; i < this.tiles * this.tiles; i++)
			sol[i] = 0;

		var presses_in_region = [];
		for(var i = 0; i < this.region_size.length; i++)
			presses_in_region[i] = 0;

		var sym = Math.floor(3 * GLib.random_double());

		var presses_left = Math.min(solution_length, this.max_solution_length);

		while(presses_left > 0)
		{
			// Pick a spot (i[0], j[0]), a corner if one is needed
			var i = [], j = [];
			i[0] = Math.round((this.tiles - 1) * GLib.random_double());
			j[0] = Math.round((this.tiles - 1) * GLib.random_double());
			// Also pick a symmetric spot, to take if possible
			var x_sym = (this.tiles - 1) - i[0];
			var y_sym = (this.tiles - 1) - j[0];
			if(sym == 0)
				i[1] = x_sym, j[1] = j [0];
			else if(sym == 1)
				i[1] = x_sym, j[1] = y_sym;
			else
				i[1] = i [0], j[1] = y_sym;
			
			for(var k = 0; k < 2; ++k)
			// Make each move if it doesn't fill a region more than halfway.
			{
				var r = this.region_of[ this.tiles * j[k] + i[k] ];
				if(r == 0 || 2*(presses_in_region[r]+1) <= this.region_size[r])
				{
					if(sol[ this.tiles * j[k] + i[k] ] != 0)
						continue;
					sol[ this.tiles * j[k] + i[k] ] = 1;
					presses_in_region[r]++;
					presses_left--;
				}
				if(presses_left == 0)
					break;
			}
		}
		return sol;
	}
}

