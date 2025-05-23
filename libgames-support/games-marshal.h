
#ifndef __games_marshal_MARSHAL_H__
#define __games_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:STRING,STRING (games-marshal.list:1) */
G_GNUC_INTERNAL void games_marshal_VOID__STRING_STRING (GClosure     *closure,
                                                        GValue       *return_value,
                                                        guint         n_param_values,
                                                        const GValue *param_values,
                                                        gpointer      invocation_hint,
                                                        gpointer      marshal_data);

/* BOOLEAN:ENUM,INT (games-marshal.list:2) */
G_GNUC_INTERNAL void games_marshal_BOOLEAN__ENUM_INT (GClosure     *closure,
                                                      GValue       *return_value,
                                                      guint         n_param_values,
                                                      const GValue *param_values,
                                                      gpointer      invocation_hint,
                                                      gpointer      marshal_data);

/* BOOLEAN:STRING,UINT,ENUM (games-marshal.list:3) */
G_GNUC_INTERNAL void games_marshal_BOOLEAN__STRING_UINT_ENUM (GClosure     *closure,
                                                              GValue       *return_value,
                                                              guint         n_param_values,
                                                              const GValue *param_values,
                                                              gpointer      invocation_hint,
                                                              gpointer      marshal_data);

G_END_DECLS

#endif /* __games_marshal_MARSHAL_H__ */

