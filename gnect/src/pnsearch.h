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


#include <time.h>


#define SEARCH_ENGINE_VERSION "V1.07"


#define MAXVALUE 200000000L

#define PROVED        1
#define DISPROVED    -1
#define UNKNOWN       0
#define UNIMPORTANT 127

#define AND_TYPE 1
#define OR_TYPE  2

#define MEM2USE (10*1024L*1024L)
#define MAXNODESNUMB 40960


struct bintree {
  struct bintree *parent;
  struct bintree *lson, *rson;
  struct node *node;
};


struct node {
  struct node *parent[7];
  struct node *child[7];
  unsigned char square[(BOARDX + 1) * (BOARDY + 2)];
  unsigned char stack[BOARDX + 1], turn, symmetric[7];
  long proof, disproof;
  short value, expanded, type, evaluated, direct;
};


struct dbtree {
  struct dbtree *parent;
  struct dbtree *lson, *rson;
  unsigned char cpos[12];
  short stacked;
  short value;
};


struct small_tree {
  struct small_tree *parent, *lson, *rson;
  unsigned char buffer[14];
};


struct parameters {
  time_t start, end;
  long maxnodes;
  long nodes, pr, ds, nodes_added;
};



struct bintree *init_bin_tree (struct node *);
struct node *check_node (struct bintree *, struct node *, short);
struct dbtree *init_dbase (unsigned char *, short);

struct bintree *fast_init_bin_tree (struct node *);
struct node *fast_check_node (struct bintree *, struct node *, short);
