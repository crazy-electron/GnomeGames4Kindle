/****************************************************************************
 *                                                                          *
 *                      Velena Source Code V1.0                             *
 *                   Written by Giuliano Bertoletti                         *
 *       Based on the knowledged approach of Louis Victor Allis             *
 *   Copyright (C) 1996-97 by Giuliano Bertoletti & GBE 32241 Software PR   *
 *                                                                          *
 ****************************************************************************

 Portable engine version.
 read the README file for further informations.

 ============================================================================

 Changes have been made to this code for inclusion with Gnect. It is
 released under the GNU General Public License with Giuliano's approval.
 The original and complete Velena Engine source code can be found at:

 http://www.ce.unipr.it/~gbe/velena.html

*/


#ifndef max
#define max(a,b) ((a)>(b) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) ((a)<(b) ? (a) : (b))
#endif

/* Prototypes !!! */

void fatal_error (char *);
short bin_compare (unsigned char *, unsigned char *);
short try_to_win (struct board *);
void free_bin_tree (struct bintree *);
short evaluation_function (struct board *);
short heuristic_search (struct node *, long);
void flush_tree (struct dbtree *, FILE *);
short makemove (struct board *, short);
short endgame (struct board *);
short ia_compute_move (struct board *);
short start_pn_search (struct board *);
short heuristic_play_best (struct board *, long);
char **allocate_matrix (struct board *);
void free_matrix (char **, struct board *);
void build_adjacency_matrix (char **, struct board *);
short problem_solver (struct board *, char **, short, FILE *);
void change_sequence (void);
int my_random (unsigned short);
long fileln (FILE *);
void initboard (struct board *board);
void fight (char t);
void expand_block (unsigned char *blk, unsigned char *pss);
short get_lower (short *bb, unsigned char *tp);
void collapse_position (unsigned char *mypos, unsigned char *blk);
short fast_try_to_win (struct board *board);
void fast_free_bin_tree (struct bintree *tree);
