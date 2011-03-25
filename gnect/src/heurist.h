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

 ===========================================================================*/


#define HE_WIN     3
#define HE_LOSS    2
#define HE_DRAW    1
#define HE_UNKNOWN 0


struct info {
  short max_tree_depth;
  short bestmove;
};
