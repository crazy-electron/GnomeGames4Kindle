#ifndef SOUND_H
#define SOUND_H


/**********************************************************************/
/* Sound Defines                                                      */
/**********************************************************************/
#define SOUND_VICTORY  0
#define SOUND_DIE      1
#define SOUND_TELEPORT 2
#define SOUND_SPLAT    3
#define SOUND_BAD      4
#define SOUND_YAHOO    5
#define NUM_SOUNDS     6
/**********************************************************************/


/**********************************************************************/
/* Exported functions                                                 */
/**********************************************************************/
gboolean init_sound (void);
gboolean play_sound (gint);
/**********************************************************************/

#endif /* SOUND_H */
