/*
 * gbdefs.h - 'DEFINES' used through out the game
 */
#ifndef GBDEFS_H
#define GBDEFS_H

/*
 * Nmae of the Game
 */
#define GAME_NAME    "gnobots2"

/*
 * Bubble Sizes
 */
#define BUBBLE_WIDTH   86
#define BUBBLE_HEIGHT  34
#define BUBBLE_XOFFSET 8
#define BUBBLE_YOFFSET 4

/*
 * Size of the game playing area
 */
#define GAME_WIDTH   45
#define GAME_HEIGHT  30

/*
 * Initial player position
 */
#define PLAYER_DEF_XPOS (GAME_WIDTH/2)
#define PLAYER_DEF_YPOS (GAME_HEIGHT/2)

/*
 * Scenario pixmaps
 */
#define SCENARIO_PIXMAP_WIDTH 14
#define SCENARIO_PLAYER_START 0
#define SCENARIO_ROBOT1_START 5
#define SCENARIO_ROBOT2_START 9
#define SCENARIO_HEAP_POS     13

#define NUM_ROBOT_ANIMATIONS  4
#define NUM_PLAYER_ANIMATIONS 4
#define PLAYER_WAVE_WAIT      20
#define PLAYER_NUM_WAVES      2

/*
 * Animation
 */
#define ANIMATION_DELAY       100
#define DEAD_DELAY            20
#define CHANGE_DELAY          20
#define WAITING_DELAY         1

/*
 * Maximums for objects
 */
#define MAX_ROBOTS ((GAME_WIDTH*GAME_HEIGHT)/2)
#define MAX_HEAPS  (MAX_ROBOTS/2)

/*
 * Game object types
 */
#define OBJECT_PLAYER 0
#define OBJECT_HEAP   1
#define OBJECT_ROBOT1 2
#define OBJECT_ROBOT2 3
#define OBJECT_NONE   99
#define OBJECT_FOO    666

/*
 * Bubble Types
 */
#define BUBBLE_NONE  0
#define BUBBLE_YAHOO 1
#define BUBBLE_AIEEE 2
#define BUBBLE_SPLAT 3

/*
 * Game states
 */
#define STATE_PLAYING     1
#define STATE_WAITING     2
#define STATE_COMPLETE    3
#define STATE_DEAD        4
#define STATE_ROBOT       5
#define STATE_TYPE2       6
#define STATE_WTYPE2      7

/*
 * Keyboard Controls
 */
#define KBD_NW   0
#define KBD_N    1
#define KBD_NE   2
#define KBD_W    3
#define KBD_STAY 4
#define KBD_E    5
#define KBD_SW   6
#define KBD_S    7
#define KBD_SE   8
#define KBD_TELE 9
#define KBD_RTEL 10
#define KBD_WAIT 11

#endif /* GBDEFS_H */
