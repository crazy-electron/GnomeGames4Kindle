/* chess-engine-cecp.c generated by valac 0.11.7, the Vala compiler
 * generated from chess-engine-cecp.vala, do not modify */


#include <glib.h>
#include <glib-object.h>
#include <string.h>
#include <stdlib.h>


#define TYPE_CHESS_ENGINE (chess_engine_get_type ())
#define CHESS_ENGINE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_CHESS_ENGINE, ChessEngine))
#define CHESS_ENGINE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_CHESS_ENGINE, ChessEngineClass))
#define IS_CHESS_ENGINE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_CHESS_ENGINE))
#define IS_CHESS_ENGINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_CHESS_ENGINE))
#define CHESS_ENGINE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_CHESS_ENGINE, ChessEngineClass))

typedef struct _ChessEngine ChessEngine;
typedef struct _ChessEngineClass ChessEngineClass;
typedef struct _ChessEnginePrivate ChessEnginePrivate;

#define TYPE_CHESS_MOVE (chess_move_get_type ())
#define CHESS_MOVE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_CHESS_MOVE, ChessMove))
#define CHESS_MOVE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_CHESS_MOVE, ChessMoveClass))
#define IS_CHESS_MOVE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_CHESS_MOVE))
#define IS_CHESS_MOVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_CHESS_MOVE))
#define CHESS_MOVE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_CHESS_MOVE, ChessMoveClass))

typedef struct _ChessMove ChessMove;
typedef struct _ChessMoveClass ChessMoveClass;

#define TYPE_CHESS_ENGINE_CECP (chess_engine_cecp_get_type ())
#define CHESS_ENGINE_CECP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_CHESS_ENGINE_CECP, ChessEngineCECP))
#define CHESS_ENGINE_CECP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_CHESS_ENGINE_CECP, ChessEngineCECPClass))
#define IS_CHESS_ENGINE_CECP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_CHESS_ENGINE_CECP))
#define IS_CHESS_ENGINE_CECP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_CHESS_ENGINE_CECP))
#define CHESS_ENGINE_CECP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_CHESS_ENGINE_CECP, ChessEngineCECPClass))

typedef struct _ChessEngineCECP ChessEngineCECP;
typedef struct _ChessEngineCECPClass ChessEngineCECPClass;
typedef struct _ChessEngineCECPPrivate ChessEngineCECPPrivate;
#define _g_free0(var) (var = (g_free (var), NULL))

struct _ChessEngine {
	GObject parent_instance;
	ChessEnginePrivate * priv;
	gchar* binary;
};

struct _ChessEngineClass {
	GObjectClass parent_class;
	void (*process_input) (ChessEngine* self, gchar* data, int data_length1);
	void (*start_game) (ChessEngine* self);
	void (*request_move) (ChessEngine* self);
	void (*report_move) (ChessEngine* self, ChessMove* move);
	void (*undo) (ChessEngine* self);
};

struct _ChessEngineCECP {
	ChessEngine parent_instance;
	ChessEngineCECPPrivate * priv;
};

struct _ChessEngineCECPClass {
	ChessEngineClass parent_class;
};

struct _ChessEngineCECPPrivate {
	gchar* buffer;
	gint buffer_length1;
	gint _buffer_size_;
	gboolean moving;
	gchar** options;
	gint options_length1;
	gint _options_size_;
};


static gpointer chess_engine_cecp_parent_class = NULL;

GType chess_engine_get_type (void) G_GNUC_CONST;
gpointer chess_move_ref (gpointer instance);
void chess_move_unref (gpointer instance);
GParamSpec* param_spec_chess_move (const gchar* name, const gchar* nick, const gchar* blurb, GType object_type, GParamFlags flags);
void value_set_chess_move (GValue* value, gpointer v_object);
void value_take_chess_move (GValue* value, gpointer v_object);
gpointer value_get_chess_move (const GValue* value);
GType chess_move_get_type (void) G_GNUC_CONST;
GType chess_engine_cecp_get_type (void) G_GNUC_CONST;
#define CHESS_ENGINE_CECP_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_CHESS_ENGINE_CECP, ChessEngineCECPPrivate))
enum  {
	CHESS_ENGINE_CECP_DUMMY_PROPERTY
};
ChessEngineCECP* chess_engine_cecp_new (gchar** options, int options_length1);
ChessEngineCECP* chess_engine_cecp_construct (GType object_type, gchar** options, int options_length1);
ChessEngine* chess_engine_new (void);
ChessEngine* chess_engine_construct (GType object_type);
static gchar** _vala_array_dup5 (gchar** self, int length);
static void chess_engine_cecp_start_cb (ChessEngineCECP* self);
static void _chess_engine_cecp_start_cb_chess_engine_starting (ChessEngine* _sender, gpointer self);
void chess_engine_write_line (ChessEngine* self, const gchar* line);
void chess_engine_set_ready (ChessEngine* self, gboolean value);
static void chess_engine_cecp_real_process_input (ChessEngine* base, gchar* data, int data_length1);
static gchar* _vala_array_dup6 (gchar* self, int length);
static void chess_engine_cecp_real_start_game (ChessEngine* base);
static void chess_engine_cecp_real_request_move (ChessEngine* base);
static void chess_engine_cecp_real_report_move (ChessEngine* base, ChessMove* move);
gchar* chess_move_get_engine (ChessMove* self);
static void chess_engine_cecp_real_undo (ChessEngine* base);
static void chess_engine_cecp_finalize (GObject* obj);
static void _vala_array_destroy (gpointer array, gint array_length, GDestroyNotify destroy_func);
static void _vala_array_free (gpointer array, gint array_length, GDestroyNotify destroy_func);


static gchar** _vala_array_dup5 (gchar** self, int length) {
	gchar** result;
	int i;
	result = g_new0 (gchar*, length + 1);
	for (i = 0; i < length; i++) {
		result[i] = g_strdup (self[i]);
	}
	return result;
}


static void _chess_engine_cecp_start_cb_chess_engine_starting (ChessEngine* _sender, gpointer self) {
	chess_engine_cecp_start_cb (self);
}


ChessEngineCECP* chess_engine_cecp_construct (GType object_type, gchar** options, int options_length1) {
	ChessEngineCECP * self = NULL;
	gchar** _tmp0_;
	gchar** _tmp1_;
	self = (ChessEngineCECP*) chess_engine_construct (object_type);
	_tmp0_ = (_tmp1_ = options, (_tmp1_ == NULL) ? ((gpointer) _tmp1_) : _vala_array_dup5 (_tmp1_, options_length1));
	self->priv->options = (_vala_array_free (self->priv->options, self->priv->options_length1, (GDestroyNotify) g_free), NULL);
	self->priv->options = _tmp0_;
	self->priv->options_length1 = options_length1;
	self->priv->_options_size_ = options_length1;
	g_signal_connect_object ((ChessEngine*) self, "starting", (GCallback) _chess_engine_cecp_start_cb_chess_engine_starting, self, 0);
	return self;
}


ChessEngineCECP* chess_engine_cecp_new (gchar** options, int options_length1) {
	return chess_engine_cecp_construct (TYPE_CHESS_ENGINE_CECP, options, options_length1);
}


static void chess_engine_cecp_start_cb (ChessEngineCECP* self) {
	g_return_if_fail (self != NULL);
	chess_engine_write_line ((ChessEngine*) self, "xboard");
	{
		gchar** o_collection;
		int o_collection_length1;
		int o_it;
		o_collection = self->priv->options;
		o_collection_length1 = self->priv->options_length1;
		for (o_it = 0; o_it < self->priv->options_length1; o_it = o_it + 1) {
			gchar* _tmp0_;
			gchar* o;
			_tmp0_ = g_strdup (o_collection[o_it]);
			o = _tmp0_;
			{
				chess_engine_write_line ((ChessEngine*) self, o);
				_g_free0 (o);
			}
		}
	}
	chess_engine_set_ready ((ChessEngine*) self, TRUE);
}


static gchar* string_slice (const gchar* self, glong start, glong end) {
	gchar* result = NULL;
	gint _tmp0_;
	glong string_length;
	gboolean _tmp1_ = FALSE;
	gboolean _tmp2_ = FALSE;
	gchar* _tmp3_ = NULL;
	g_return_val_if_fail (self != NULL, NULL);
	_tmp0_ = strlen (self);
	string_length = (glong) _tmp0_;
	if (start < 0) {
		start = string_length + start;
	}
	if (end < 0) {
		end = string_length + end;
	}
	if (start >= 0) {
		_tmp1_ = start <= string_length;
	} else {
		_tmp1_ = FALSE;
	}
	g_return_val_if_fail (_tmp1_, NULL);
	if (end >= 0) {
		_tmp2_ = end <= string_length;
	} else {
		_tmp2_ = FALSE;
	}
	g_return_val_if_fail (_tmp2_, NULL);
	g_return_val_if_fail (start <= end, NULL);
	_tmp3_ = g_strndup (((gchar*) self) + start, (gsize) (end - start));
	result = _tmp3_;
	return result;
}


static gchar* _vala_array_dup6 (gchar* self, int length) {
	return g_memdup (self, length * sizeof (gchar));
}


static void chess_engine_cecp_real_process_input (ChessEngine* base, gchar* data, int data_length1) {
	ChessEngineCECP * self;
	gint current;
	gint _tmp0_;
	self = (ChessEngineCECP*) base;
	current = self->priv->buffer_length1;
	_tmp0_ = (gint) (self->priv->buffer_length1 + data_length1);
	self->priv->buffer = g_renew (gchar, self->priv->buffer, (gint) (self->priv->buffer_length1 + data_length1));
	(_tmp0_ > self->priv->buffer_length1) ? memset (self->priv->buffer + self->priv->buffer_length1, 0, sizeof (gchar) * (_tmp0_ - self->priv->buffer_length1)) : NULL;
	self->priv->buffer_length1 = _tmp0_;
	self->priv->_buffer_size_ = _tmp0_;
	{
		gint i;
		i = 0;
		{
			gboolean _tmp1_;
			_tmp1_ = TRUE;
			while (TRUE) {
				if (!_tmp1_) {
					i++;
				}
				_tmp1_ = FALSE;
				if (!(i < data_length1)) {
					break;
				}
				self->priv->buffer[current + i] = data[i];
			}
		}
	}
	while (TRUE) {
		gint offset = 0;
		gchar* _tmp4_;
		gchar* line;
		gchar* _tmp5_;
		gchar* _tmp6_;
		gchar* _tmp7_;
		gchar** _tmp8_ = NULL;
		gchar** move_prefixes;
		gint move_prefixes_length1;
		gint _move_prefixes_size_;
		gboolean _tmp14_;
		gchar* _tmp16_;
		gchar* _tmp17_;
		{
			gboolean _tmp2_;
			offset = 0;
			_tmp2_ = TRUE;
			while (TRUE) {
				gboolean _tmp3_ = FALSE;
				if (!_tmp2_) {
					offset++;
				}
				_tmp2_ = FALSE;
				if (offset < self->priv->buffer_length1) {
					_tmp3_ = self->priv->buffer[offset] != '\n';
				} else {
					_tmp3_ = FALSE;
				}
				if (!_tmp3_) {
					break;
				}
			}
		}
		if (offset >= self->priv->buffer_length1) {
			return;
		}
		self->priv->buffer[offset] = '\0';
		_tmp4_ = g_strdup ((const gchar*) self->priv->buffer);
		line = _tmp4_;
		g_debug ("chess-engine-cecp.vala:41: Read from engine: '%s'", line);
		_tmp5_ = g_strdup ("My move is: ");
		_tmp6_ = g_strdup ("my move is ");
		_tmp7_ = g_strdup ("move ");
		_tmp8_ = g_new0 (gchar*, 3 + 1);
		_tmp8_[0] = _tmp5_;
		_tmp8_[1] = _tmp6_;
		_tmp8_[2] = _tmp7_;
		move_prefixes = _tmp8_;
		move_prefixes_length1 = 3;
		_move_prefixes_size_ = 3;
		{
			gchar** prefix_collection;
			int prefix_collection_length1;
			int prefix_it;
			prefix_collection = move_prefixes;
			prefix_collection_length1 = move_prefixes_length1;
			for (prefix_it = 0; prefix_it < move_prefixes_length1; prefix_it = prefix_it + 1) {
				gchar* _tmp9_;
				gchar* prefix;
				_tmp9_ = g_strdup (prefix_collection[prefix_it]);
				prefix = _tmp9_;
				{
					gboolean _tmp10_;
					_tmp10_ = g_str_has_prefix (line, prefix);
					if (_tmp10_) {
						gint _tmp11_;
						gint _tmp12_;
						gchar* _tmp13_ = NULL;
						gchar* move;
						_tmp11_ = strlen (prefix);
						_tmp12_ = strlen (line);
						_tmp13_ = string_slice (line, (glong) _tmp11_, (glong) _tmp12_);
						move = _tmp13_;
						g_debug ("chess-engine-cecp.vala:49: Engine moves %s", move);
						self->priv->moving = TRUE;
						g_signal_emit_by_name ((ChessEngine*) self, "moved", move);
						_g_free0 (move);
					}
					_g_free0 (prefix);
				}
			}
		}
		_tmp14_ = g_str_has_prefix (line, "Illegal move: ");
		if (_tmp14_) {
		} else {
			gboolean _tmp15_ = FALSE;
			if (g_strcmp0 (line, "resign") == 0) {
				_tmp15_ = TRUE;
			} else {
				_tmp15_ = g_strcmp0 (line, "tellics resign") == 0;
			}
			if (_tmp15_) {
			} else {
				if (g_strcmp0 (line, "offer draw") == 0) {
				}
			}
		}
		_tmp16_ = (_tmp17_ = self->priv->buffer + (offset + 1), (_tmp17_ == NULL) ? ((gpointer) _tmp17_) : _vala_array_dup6 (_tmp17_, self->priv->buffer_length1 - (offset + 1)));
		self->priv->buffer = (g_free (self->priv->buffer), NULL);
		self->priv->buffer = _tmp16_;
		self->priv->buffer_length1 = self->priv->buffer_length1 - (offset + 1);
		self->priv->_buffer_size_ = self->priv->buffer_length1 - (offset + 1);
		move_prefixes = (_vala_array_free (move_prefixes, move_prefixes_length1, (GDestroyNotify) g_free), NULL);
		_g_free0 (line);
	}
}


static void chess_engine_cecp_real_start_game (ChessEngine* base) {
	ChessEngineCECP * self;
	self = (ChessEngineCECP*) base;
}


static void chess_engine_cecp_real_request_move (ChessEngine* base) {
	ChessEngineCECP * self;
	self = (ChessEngineCECP*) base;
	chess_engine_write_line ((ChessEngine*) self, "go");
}


static void chess_engine_cecp_real_report_move (ChessEngine* base, ChessMove* move) {
	ChessEngineCECP * self;
	self = (ChessEngineCECP*) base;
	g_return_if_fail (move != NULL);
	if (!self->priv->moving) {
		gchar* _tmp0_ = NULL;
		gchar* _tmp1_;
		chess_engine_write_line ((ChessEngine*) self, "force");
		_tmp0_ = chess_move_get_engine (move);
		_tmp1_ = _tmp0_;
		chess_engine_write_line ((ChessEngine*) self, _tmp1_);
		_g_free0 (_tmp1_);
	}
	self->priv->moving = FALSE;
}


static void chess_engine_cecp_real_undo (ChessEngine* base) {
	ChessEngineCECP * self;
	self = (ChessEngineCECP*) base;
	chess_engine_write_line ((ChessEngine*) self, "undo");
}


static void chess_engine_cecp_class_init (ChessEngineCECPClass * klass) {
	chess_engine_cecp_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (ChessEngineCECPPrivate));
	CHESS_ENGINE_CLASS (klass)->process_input = chess_engine_cecp_real_process_input;
	CHESS_ENGINE_CLASS (klass)->start_game = chess_engine_cecp_real_start_game;
	CHESS_ENGINE_CLASS (klass)->request_move = chess_engine_cecp_real_request_move;
	CHESS_ENGINE_CLASS (klass)->report_move = chess_engine_cecp_real_report_move;
	CHESS_ENGINE_CLASS (klass)->undo = chess_engine_cecp_real_undo;
	G_OBJECT_CLASS (klass)->finalize = chess_engine_cecp_finalize;
}


static void chess_engine_cecp_instance_init (ChessEngineCECP * self) {
	self->priv = CHESS_ENGINE_CECP_GET_PRIVATE (self);
	self->priv->moving = FALSE;
}


static void chess_engine_cecp_finalize (GObject* obj) {
	ChessEngineCECP * self;
	self = CHESS_ENGINE_CECP (obj);
	self->priv->buffer = (g_free (self->priv->buffer), NULL);
	self->priv->options = (_vala_array_free (self->priv->options, self->priv->options_length1, (GDestroyNotify) g_free), NULL);
	G_OBJECT_CLASS (chess_engine_cecp_parent_class)->finalize (obj);
}


GType chess_engine_cecp_get_type (void) {
	static volatile gsize chess_engine_cecp_type_id__volatile = 0;
	if (g_once_init_enter (&chess_engine_cecp_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (ChessEngineCECPClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) chess_engine_cecp_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (ChessEngineCECP), 0, (GInstanceInitFunc) chess_engine_cecp_instance_init, NULL };
		GType chess_engine_cecp_type_id;
		chess_engine_cecp_type_id = g_type_register_static (TYPE_CHESS_ENGINE, "ChessEngineCECP", &g_define_type_info, 0);
		g_once_init_leave (&chess_engine_cecp_type_id__volatile, chess_engine_cecp_type_id);
	}
	return chess_engine_cecp_type_id__volatile;
}


static void _vala_array_destroy (gpointer array, gint array_length, GDestroyNotify destroy_func) {
	if ((array != NULL) && (destroy_func != NULL)) {
		int i;
		for (i = 0; i < array_length; i = i + 1) {
			if (((gpointer*) array)[i] != NULL) {
				destroy_func (((gpointer*) array)[i]);
			}
		}
	}
}


static void _vala_array_free (gpointer array, gint array_length, GDestroyNotify destroy_func) {
	_vala_array_destroy (array, array_length, destroy_func);
	g_free (array);
}



