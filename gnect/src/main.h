/* main.h */


#define APPNAME "gnect"
#define APPNAME_LONG N_("Four-in-a-row")

#define TILE_PLAYER1           0
#define TILE_PLAYER2           1
#define TILE_CLEAR             2
#define TILE_CLEAR_CURSOR      3
#define TILE_PLAYER1_CURSOR    4
#define TILE_PLAYER2_CURSOR    5

#define SIZE_VSTR      53

#define MAIN_PAGE           	0
#define NETWORK_PAGE           	1

typedef enum {
  MOVE_LEFT,
  MOVE_RIGHT,
  MOVE_DROP
} MoveID;

typedef enum {
  STATUS_CLEAR,
  STATUS_FLASH,
  STATUS_SET
} StatusID;

typedef enum {
  SOUND_DROP,
  SOUND_I_WIN,
  SOUND_YOU_WIN,
  SOUND_PLAYER_WIN,
  SOUND_DRAWN_GAME,
  SOUND_COLUMN_FULL
} SoundID;

typedef enum {
  PLAYER1,
  PLAYER2,
  NOBODY
} PlayerID;

typedef enum {
  LEVEL_HUMAN,
  LEVEL_WEAK,
  LEVEL_MEDIUM,
  LEVEL_STRONG
} LevelID;

extern PlayerID who_starts;
extern GtkWidget *app;
extern GtkWidget *notebook;

gboolean player_active;
gboolean ggz_network_mode;

void game_reset (void);
void process_move (int move);
void prompt_player (void);
void on_dialog_close (GtkWidget * w, int response_id, gpointer data);
void scorebox_update (void);
void scorebox_reset (void);
void set_status_message (const gchar * message);
