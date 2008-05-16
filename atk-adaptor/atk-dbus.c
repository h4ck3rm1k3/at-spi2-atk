/*
 * AT-SPI - Assistive Technology Service Provider Interface
 * (Gnome Accessibility Project; http://developer.gnome.org/projects/gap)
 *
 * Copyright 2008 Novell, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "accessible.h"

extern GHashTable *path2ptr;

AtkObject *
spi_dbus_get_object (const char *path)
{
  guint index;
  void *data;

  g_assert (path);
  if (strncmp(path, "/org/freedesktop/atspi/accessible", 33) != 0) return NULL;
  path += 33;	/* skip over preamble */
  if (path[0] == '\0') return atk_get_root();
  if (path[0] != '/') return NULL;
  path++;
  index = atoi (path);
  data = g_hash_table_lookup (path2ptr, &index);
  if (data)
    return ATK_OBJECT (data);
  return NULL;
}

gchar *
spi_dbus_get_path (AtkObject * obj)
{
  if (!obj) return NULL;
  guint index = (guint) g_object_get_data (G_OBJECT (obj), "dbus-id");
  if (!index)
    index = register_object (obj);
  return g_strdup_printf ("/org/freedesktop/atspi/accessible/%d", index);
}

/* Reply with the given object and dereference it if unref is TRUE */
DBusMessage *
spi_dbus_return_object (DBusMessage * message, AtkObject * obj, int unref)
{
  DBusMessage *reply;
  const char *path = spi_dbus_get_path (obj);
  if (unref)
    g_object_unref (obj);
  if (!path)
    {
      /* Should we have a more specific error for this? */
      return spi_dbus_general_error (message);
    }
  reply = dbus_message_new_method_return (message);
  if (reply)
    {
      dbus_message_append_args (reply, DBUS_TYPE_OBJECT_PATH, path,
				DBUS_TYPE_INVALID);
    }
  return reply;
}

dbus_bool_t
spi_dbus_return_v_object (DBusMessageIter * iter, AtkObject * obj, int unref)
{
  const char *path = spi_dbus_get_path (obj);
  if (unref)
    g_object_unref (obj);
  if (!path)
    return FALSE;
  return droute_return_v_object (iter, path);
}

void
spi_dbus_initialize (DRouteData * data)
{
  spi_initialize_accessible (data);
  spi_initialize_action(data);
  spi_initialize_component (data);
  spi_initialize_document (data);
  spi_initialize_editabletext (data);
  spi_initialize_hyperlink (data);
  spi_initialize_hypertext (data);
  spi_initialize_image (data);
  spi_initialize_selection (data);
  spi_initialize_table (data);
  spi_initialize_text (data);
  spi_initialize_value (data);
  spi_initialize_introspectable(data);
}

static GString *
spi_get_tree (AtkObject * obj, GString * str, DRouteData * data)
{
  int role;
  const char *name;
  gchar *path;
  GSList *l;
  gint childcount;
  gint i;

  if (!obj)
    return NULL;
  role = spi_accessible_role_from_atk_role (atk_object_get_role (obj));;
  name = atk_object_get_name (obj);
  if (!name)
    name = "";
  path = spi_dbus_get_path (obj);
  g_string_append_printf (str,
			  "<object path=\"%s\" name=\"%s\" role=\"%d\">\n",
			  path, name, role);
  for (l = data->interfaces; l; l = g_slist_next (l))
    {
      DRouteInterface *iface_def = (DRouteInterface *) l->data;
      void *datum = NULL;
      if (iface_def->get_datum)
	datum = (*iface_def->get_datum) (path, data->user_data);
      if (datum)
	{
	  g_string_append_printf (str, "<interface name=\"%s\"/>\n",
				  iface_def->name);
	  if (iface_def->free_datum)
	    (*iface_def->free_datum) (datum);
	}
    }
  childcount = atk_object_get_n_accessible_children (obj);
  for (i = 0; i < childcount; i++)
    {
      AtkObject *child = atk_object_ref_accessible_child (obj, i);
      str = spi_get_tree (child, str, data);
      g_object_unref (child);
    }
  str = g_string_append (str, "</object>\n");
  return str;
}