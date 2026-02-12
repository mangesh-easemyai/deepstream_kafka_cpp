#ifndef JSON_CONFIG_READ_H
#define JSON_CONFIG_READ_H


#include <glib.h>
#include <json-glib/json-glib.h>


gboolean json_glib_create_config(const gchar *json_file,const gchar *output_config,GError **error);

#endif