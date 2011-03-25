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


#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "connect4.h"
#include "con4vals.h"
#include "rules.h"
#include "pnsearch.h"
#include "proto.h"

#include "heurist.h"


/* FIXME: These should probably go into a header file or be defined as static */
void fight (char t);
short whatfight (void);
void her_generate_all_children (struct node *node);
void her_free_whole_tree (struct bintree *tree);
short her_evaluate (struct node *node);
short her_resources_available (long maxnodes);
struct node *her_select_most_proving_node (struct node *node,
					   struct info *info);
void her_set_pdv_according_to_children (struct node *node);
void her_set_proof_and_disproof_numbers (struct node *node);
void her_develop_node (struct node *node);
void her_update_ancestors (struct node *node);
void her_pn_search (struct node *root, long maxnodes, struct info *info);
short heuristic_search (struct node *mynode, long maxnodenum);
short heuristic_play_best (struct board *board, long maxnodenum);


extern short nodeseq[];
struct board *auxboard;


struct bintree *streeroot;
struct node *rootnode;

short nodeseq[] = { 3, 2, 4, 5, 1, 0, 6 };	/* This is the sequence we follow to
						 * scan the seven moves available
						 */

char fight_for_win = 0;



void
fight (char t)
{
  fight_for_win = t;
}



short
whatfight (void)
{
  return (short) fight_for_win;
}



long her_node_expanded, her_node_not_expanded;



void
her_generate_all_children (struct node *node)
{
  struct node *dummy, *symm;
  short x, y, x1, y1, backtrace;


  for (x = 0; x < BOARDX; x++) {
    if (node->child[x])
      fatal_error ("Tried to allocate a node twice!");
    else if (node->stack[x] < BOARDY) {
      node->child[x] = (struct node *) malloc (sizeof (struct node));
      symm = (struct node *) malloc (sizeof (struct node));

      if (!node->child[x] || !symm)
	fatal_error ("Not enough memory to build the tree");

      memcpy (node->child[x]->square, node->square,
	      (BOARDX + 1) * (BOARDY + 2));
      memcpy (node->child[x]->stack, node->stack, BOARDX + 1);
      backtrace = x;

      node->child[x]->square[ELM (x, node->child[x]->stack[x]++)] =
	node->turn;
      node->child[x]->turn = node->turn ^ SWITCHSIDE;
      symm->turn = node->turn ^ SWITCHSIDE;

      for (y1 = 0; y1 < BOARDY; y1++) {
	symm->square[ELM (BOARDX, y1)] =
	  node->child[x]->square[ELM (BOARDX, y1)];
	for (x1 = 0; x1 < BOARDX; x1++)
	  symm->square[ELM (x1, y1)] =
	    node->child[x]->square[ELM (BOARDX - x1 - 1, y1)];
      }

      symm->stack[BOARDX] = node->child[x]->stack[BOARDX];
      for (x1 = 0; x1 < BOARDX; x1++)
	symm->stack[x1] = node->child[x]->stack[BOARDX - x1 - 1];

      if (bin_compare (symm->square, node->child[x]->square) > 0) {
	dummy = symm;
	symm = node->child[x];
	node->child[x] = dummy;
	backtrace = BOARDX - x - 1;
      }

      dummy = fast_check_node (streeroot, node->child[x], 0);
      if (dummy) {
	free (node->child[x]);
	node->child[x] = dummy;
	node->child[x]->parent[backtrace] = node;
	her_node_not_expanded++;
      } else {
	node->child[x]->expanded = NO;
	node->child[x]->evaluated = NO;

	for (y = 0; y < BOARDX; y++)
	  node->child[x]->parent[y] = NULL;

	node->child[x]->parent[backtrace] = node;

	for (y = 0; y < BOARDX; y++)
	  node->child[x]->child[y] = NULL;

	her_node_expanded++;

	if (node->type == AND_TYPE)
	  node->child[x]->type = OR_TYPE;
	else if (node->type == OR_TYPE)
	  node->child[x]->type = AND_TYPE;
	else
	  fatal_error ("Could not recognize node type!");

	/*add_node(streeroot,node->child[x]); */
      }

      node->symmetric[x] = backtrace;
      free (symm);
    }

    else
      node->child[x] = NULL;
  }
}



void
her_free_whole_tree (struct bintree *tree)
{
  if (!tree)
    return;
  if (tree->lson)
    her_free_whole_tree (tree->lson);
  if (tree->rson)
    her_free_whole_tree (tree->rson);

  free (tree->node);
}



short
her_evaluate (struct node *node)
{
  short x, bestmove;


  node->evaluated = YES;

  for (x = 0; x < 64; x++)
    auxboard->square[x] = (short) node->square[x];

  auxboard->turn = node->turn;
  auxboard->filled = 0;

  for (x = 0; x < BOARDX + 1; x++) {
    auxboard->stack[x] = (short) node->stack[x];
    if (x < BOARDX)
      auxboard->filled += auxboard->stack[x];
  }

  if (auxboard->filled == MAXMEN) {
    if (rootnode->turn == WHITE) {
      if (!fight_for_win) {
	if (rootnode->type == OR_TYPE)
	  node->value = DISPROVED;
	else
	  node->value = PROVED;
      } else {
	if (rootnode->type == OR_TYPE)
	  node->value = PROVED;
	else
	  node->value = DISPROVED;
      }
    } else {
      if (!fight_for_win) {
	if (rootnode->type == OR_TYPE)
	  node->value = PROVED;
	else
	  node->value = DISPROVED;
      } else {
	if (rootnode->type == OR_TYPE)
	  node->value = DISPROVED;
	else
	  node->value = PROVED;
      }
    }
    return -1;
  }

  bestmove = fast_try_to_win (auxboard);
  if (bestmove != -1) {
    if (node->type == OR_TYPE)
      node->value = PROVED;
    if (node->type == AND_TYPE)
      node->value = DISPROVED;
  }

  else
    node->value = UNKNOWN;

  return bestmove;
}



short
her_resources_available (long maxnodes)
{
  if (her_node_expanded > maxnodes)
    return NO;
  return YES;
}



struct node *
her_select_most_proving_node (struct node *node, struct info *info)
{
  short i, flag, depth = 0;


  while (node->expanded) {
    i = 0;
    flag = FALSE;

    switch (node->type) {
    case OR_TYPE:
      do {
	if (node->child[nodeseq[i]] &&
	    node->child[nodeseq[i]]->proof == node->proof)
	  flag = TRUE;
	else
	  i++;
      } while (i < BOARDX && !flag);

      if (!flag)
	fatal_error ("Could not find a good OR child!");
      break;

    case AND_TYPE:
      do {
	if (node->child[nodeseq[i]] &&
	    node->child[nodeseq[i]]->disproof == node->disproof)
	  flag = TRUE;
	else
	  i++;
      } while (i < BOARDX && !flag);

      if (!flag)
	fatal_error ("Could not find a good AND child!");
      break;

    default:
      fatal_error ("Unknown node type!");
      break;

    }
    if (!flag)
      fatal_error ("Null node found!");
    else {
      if (depth == 0)
	info->bestmove = nodeseq[i];
      node = node->child[nodeseq[i]];
    }

    depth++;
  }

  info->max_tree_depth = max (info->max_tree_depth, depth + 1);
  return node;
}



void
her_set_pdv_according_to_children (struct node *node)
{
  short x, children = 0;


  for (x = 0; x < BOARDX; x++)
    if (node->stack[x] < BOARDY)
      children++;

  if (children == 0)
    fatal_error ("Father without children but of value unknown");

  switch (node->type) {
  case AND_TYPE:
    node->proof = children;
    node->disproof = 1;
    break;

  case OR_TYPE:
    node->proof = 1;
    node->disproof = children;
    break;
  }
}



void
her_set_proof_and_disproof_numbers (struct node *node)
{
  short x;


  if (!node)
    fatal_error ("Invalid node choosen");
  else if (node->expanded) {
    switch (node->type) {
    case AND_TYPE:
      node->proof = 0;
      node->disproof = MAXVALUE;
      for (x = 0; x < BOARDX; x++)
	if (node->child[x]) {
	  node->proof += node->child[x]->proof;
	  node->disproof = min (node->disproof, node->child[x]->disproof);
	}
      if (node->disproof == 0)
	node->proof = MAXVALUE;
      break;

    case OR_TYPE:
      node->proof = MAXVALUE;
      node->disproof = 0;
      for (x = 0; x < BOARDX; x++)
	if (node->child[x]) {
	  node->proof = min (node->proof, node->child[x]->proof);
	  node->disproof += node->child[x]->disproof;
	}
      if (node->proof == 0)
	node->disproof = MAXVALUE;
      break;

    default:
      fatal_error ("I don't know what to prove or disprove!");
      break;
    }

    if (node->proof < 0 || node->disproof < 0)
      fatal_error ("Proof and disproof numbers cannot be lower than zero");

  } else if (node->evaluated) {
    switch (node->value) {
    case PROVED:
      node->proof = 0;
      node->disproof = MAXVALUE;
      break;

    case DISPROVED:
      node->proof = MAXVALUE;
      node->disproof = 0;
      break;

    case UNKNOWN:
      her_set_pdv_according_to_children (node);
      break;
    }
  } else
    fatal_error ("Asked to set a node never evaluated!");
}



void
her_develop_node (struct node *node)
{
  short i;


  node->expanded = YES;
  her_generate_all_children (node);

  for (i = 0; i < BOARDX; i++) {
    if (node->child[i]) {
      her_evaluate (node->child[i]);
      her_set_proof_and_disproof_numbers (node->child[i]);
    }
  }
}



void
her_update_ancestors (struct node *node)
{
  short x;


  if (node) {
    her_set_proof_and_disproof_numbers (node);

    for (x = 0; x < BOARDX; x++)
      if (node->parent[x])
	her_update_ancestors (node->parent[x]);
  }
}



void
her_pn_search (struct node *root, long maxnodes, struct info *info)
{
  struct node *her_most_proving_node;


  info->max_tree_depth = 1;
  info->bestmove = her_evaluate (root);
  her_set_proof_and_disproof_numbers (root);

  while (root->proof != 0 && root->disproof != 0
	 && her_resources_available (maxnodes)) {
    her_most_proving_node = her_select_most_proving_node (root, info);
    her_develop_node (her_most_proving_node);
    her_update_ancestors (her_most_proving_node);
  }

  if (root->proof == 0)
    root->value = PROVED;
  else if (root->disproof == 0)
    root->value = DISPROVED;
  else
    root->value = UNKNOWN;
}



short
heuristic_search (struct node *mynode, long maxnodenum)
{
  struct info info;
  short x;


  if (mynode->expanded || mynode->evaluated)
    fatal_error ("Request to evaluate a node already evaluated or expanded!");

  mynode->evaluated = YES;

  rootnode = (struct node *) malloc (sizeof (struct node));
  if (!rootnode)
    fatal_error ("Not enough memory");

  memcpy (rootnode, mynode, sizeof (struct node));

  her_node_expanded = 0L;
  her_node_not_expanded = 0L;

  for (x = 0; x < BOARDX; x++)
    rootnode->parent[x] = NULL;

  streeroot = fast_init_bin_tree (rootnode);
  her_pn_search (rootnode, maxnodenum, &info);

  mynode->value = rootnode->value;
  mynode->proof = rootnode->proof;
  mynode->disproof = rootnode->disproof;

  her_free_whole_tree (streeroot);
  fast_free_bin_tree (streeroot);

  if (mynode->value != UNKNOWN)
    return YES;
  return NO;
}



/* Avoid tricks */

short
heuristic_play_best (struct board *board, long maxnodenum)
{
  struct board *tempboard;
  struct info info;
  short mymove;
  struct node *symmetric;
  short x, y, issymm = NO;


  auxboard = board;
  tempboard = (struct board *) malloc (sizeof (struct board));
  if (!tempboard)
    fatal_error ("Cannot store temporary board");
  memcpy (tempboard, board, sizeof (struct board));

  rootnode = (struct node *) malloc (sizeof (struct node));
  symmetric = (struct node *) malloc (sizeof (struct node));
  if (!rootnode || !symmetric)
    fatal_error ("Not enough memory");

  for (y = 0; y < BOARDY; y++) {
    rootnode->square[ELM (BOARDX, y)] =
      (unsigned char) (board->square[ELM (BOARDX, y)] & 0xff);
    symmetric->square[ELM (BOARDX, y)] =
      (unsigned char) (board->square[ELM (BOARDX, y)] & 0xff);
    for (x = 0; x < BOARDX; x++) {
      rootnode->square[ELM (x, y)] =
	(unsigned char) (board->square[ELM (x, y)] & 0xff);
      symmetric->square[ELM (x, y)] =
	(unsigned char) (board->square[ELM (BOARDX - x - 1, y)] & 0xff);
    }
  }

  rootnode->turn = board->turn;
  symmetric->turn = board->turn;

  for (x = 0; x < BOARDX; x++) {
    rootnode->stack[x] = (unsigned char) (board->stack[x] & 0xff);
    symmetric->stack[x] =
      (unsigned char) (board->stack[BOARDX - 1 - x] & 0xff);
  }

  rootnode->stack[BOARDX] = (unsigned char) (board->stack[BOARDX] & 0xff);
  symmetric->stack[BOARDX] = (unsigned char) (board->stack[BOARDX] & 0xff);

  if (bin_compare (symmetric->square, rootnode->square) > 0) {
    free (rootnode);
    rootnode = symmetric;
    issymm = YES;
  } else
    free (symmetric);

  her_node_expanded = 0L;
  her_node_not_expanded = 0L;

  for (x = 0; x < BOARDX; x++) {
    rootnode->parent[x] = NULL;
    rootnode->child[x] = NULL;
  }

  rootnode->evaluated = NO;
  rootnode->expanded = NO;
  rootnode->value = UNKNOWN;
  rootnode->type = OR_TYPE;

  streeroot = fast_init_bin_tree (rootnode);
  her_pn_search (rootnode, maxnodenum, &info);
  mymove = info.bestmove;

  if (rootnode->value == UNKNOWN)
    mymove = -1;
  else if (rootnode->value == DISPROVED)
    mymove = -2;
  else if (issymm)
    mymove = BOARDX - 1 - mymove;

  her_free_whole_tree (streeroot);
  fast_free_bin_tree (streeroot);

  memcpy (board, tempboard, sizeof (struct board));
  free (tempboard);

  board->nodes_visited = her_node_expanded + her_node_not_expanded;
  board->maxtreedepth = info.max_tree_depth;
  return mymove;
}
