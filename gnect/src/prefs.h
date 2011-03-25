/* prefs.h */

#define KEY_LEVEL_PLAYER1      "player1"
#define KEY_LEVEL_PLAYER2      "player2"
#define KEY_THEME_ID           "theme_id"
#define KEY_DO_SOUND           "sound"
#define KEY_DO_ANIMATE         "animate"

#define KEY_MOVE_LEFT          "keyleft"
#define KEY_MOVE_RIGHT         "keyright"
#define KEY_MOVE_DROP          "keydrop"

typedef struct _Prefs Prefs;
struct _Prefs {
  gboolean do_sound;
  gboolean do_animate;
  gint theme_id;
  LevelID level[2];
  gint keypress[3];
};


void prefs_init (void);
void prefsbox_open (void);
