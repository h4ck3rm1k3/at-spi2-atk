/*
 * AT-SPI - Assistive Technology Service Provider Interface
 * (Gnome Accessibility Project; http://developer.gnome.org/projects/gap)
 *
 * Copyright 2008 Novell, Inc.
 * Copyright 2001, 2002 Sun Microsystems Inc.,
 * Copyright 2001, 2002 Ximian, Inc.
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

#include <atk/atk.h>
#include <droute/droute.h>

#include "common/spi-dbus.h"
#include "common/spi-stateset.h"
#include "object.h"

static dbus_bool_t
impl_get_Name (DBusMessageIter * iter, void *user_data)
{
  AtkObject *object = (AtkObject *) user_data;

  g_return_val_if_fail (ATK_IS_OBJECT (user_data), FALSE);

  return droute_return_v_string (iter, atk_object_get_name (object));
}

static dbus_bool_t
impl_set_Name (DBusMessageIter * iter, void *user_data)
{
  AtkObject *object = (AtkObject *) user_data;
  const char *name = droute_get_v_string (iter);

  g_return_val_if_fail (ATK_IS_OBJECT (user_data), FALSE);

  atk_object_set_name (object, name);
  return TRUE;
}

static dbus_bool_t
impl_get_Description (DBusMessageIter * iter, void *user_data)
{
  AtkObject *object = (AtkObject *) user_data;

  g_return_val_if_fail (ATK_IS_OBJECT (user_data), FALSE);

  return droute_return_v_string (iter, atk_object_get_description (object));
}

static dbus_bool_t
impl_set_Description (DBusMessageIter * iter, void *user_data)
{
  AtkObject *object = (AtkObject *) user_data;
  const char *description = droute_get_v_string (iter);

  g_return_val_if_fail (ATK_IS_OBJECT (user_data), FALSE);

  atk_object_set_description (object, description);
  return TRUE;
}

static dbus_bool_t
impl_get_Parent (DBusMessageIter * iter, void *user_data)
{
  AtkObject *object = (AtkObject *) user_data;

  g_return_val_if_fail (ATK_IS_OBJECT (user_data), FALSE);

  spi_object_append_v_reference (iter, atk_object_get_parent (object));
  return TRUE;
}

static dbus_bool_t
impl_get_ChildCount (DBusMessageIter * iter, void *user_data)
{
  AtkObject *object = (AtkObject *) user_data;

  g_return_val_if_fail (ATK_IS_OBJECT (user_data), FALSE);

  return droute_return_v_int32 (iter,
                                atk_object_get_n_accessible_children
                                (object));
}

static DBusMessage *
impl_GetChildAtIndex (DBusConnection * bus,
                      DBusMessage * message, void *user_data)
{
  AtkObject *object = (AtkObject *) user_data;
  DBusError error;
  dbus_int32_t i;
  AtkObject *child;

  dbus_error_init (&error);
  g_return_val_if_fail (ATK_IS_OBJECT (user_data),
                        droute_not_yet_handled_error (message));
  if (!dbus_message_get_args 
       (message, &error, DBUS_TYPE_INT32, &i, DBUS_TYPE_INVALID))
    {
      return droute_invalid_arguments_error (message);
    }
  child = atk_object_ref_accessible_child (object, i);
  return spi_object_return_reference (message, child);
}

static DBusMessage *
impl_GetChildren (DBusConnection * bus,
                  DBusMessage * message, void *user_data)
{
  AtkObject *object = (AtkObject *) user_data;
  gint i;
  gint count;
  DBusMessage *reply;
  DBusMessageIter iter, iter_array;

  g_return_val_if_fail (ATK_IS_OBJECT (user_data),
                        droute_not_yet_handled_error (message));
  count = atk_object_get_n_accessible_children (object);
  reply = dbus_message_new_method_return (message);
  if (!reply)
    goto oom;
  dbus_message_iter_init_append (reply, &iter);
  if (!dbus_message_iter_open_container
      (&iter, DBUS_TYPE_ARRAY, "(so)", &iter_array))
    goto oom;
  for (i = 0; i < count; i++)
    {
      AtkObject *child = atk_object_ref_accessible_child (object, i);
      spi_object_append_reference (&iter_array, child); 
      if (child)
        g_object_unref (child);
    }
  if (!dbus_message_iter_close_container (&iter, &iter_array))
    goto oom;
  return reply;
oom:
  // TODO: handle out-of-memory
  return reply;
}

static DBusMessage *
impl_GetIndexInParent (DBusConnection * bus,
                       DBusMessage * message, void *user_data)
{
  AtkObject *object = (AtkObject *) user_data;
  dbus_uint32_t rv;
  DBusMessage *reply;

  g_return_val_if_fail (ATK_IS_OBJECT (user_data),
                        droute_not_yet_handled_error (message));

  rv = atk_object_get_index_in_parent (object);
  reply = dbus_message_new_method_return (message);
  dbus_message_append_args (reply, DBUS_TYPE_UINT32, &rv, DBUS_TYPE_INVALID);
  return reply;
}

static gboolean
spi_init_relation_type_table (Accessibility_RelationType * types)
{
  gint i;

  for (i = 0; i < ATK_RELATION_LAST_DEFINED; i++)
    types[i] = Accessibility_RELATION_NULL;

  types[ATK_RELATION_CONTROLLED_BY] = Accessibility_RELATION_CONTROLLED_BY;
  types[ATK_RELATION_CONTROLLER_FOR] = Accessibility_RELATION_CONTROLLER_FOR;
  types[ATK_RELATION_LABEL_FOR] = Accessibility_RELATION_LABEL_FOR;
  types[ATK_RELATION_LABELLED_BY] = Accessibility_RELATION_LABELLED_BY;
  types[ATK_RELATION_MEMBER_OF] = Accessibility_RELATION_MEMBER_OF;
  types[ATK_RELATION_NODE_CHILD_OF] = Accessibility_RELATION_NODE_CHILD_OF;
  types[ATK_RELATION_FLOWS_TO] = Accessibility_RELATION_FLOWS_TO;
  types[ATK_RELATION_FLOWS_FROM] = Accessibility_RELATION_FLOWS_FROM;
  types[ATK_RELATION_SUBWINDOW_OF] = Accessibility_RELATION_SUBWINDOW_OF;
  types[ATK_RELATION_EMBEDS] = Accessibility_RELATION_EMBEDS;
  types[ATK_RELATION_EMBEDDED_BY] = Accessibility_RELATION_EMBEDDED_BY;
  types[ATK_RELATION_POPUP_FOR] = Accessibility_RELATION_POPUP_FOR;
  types[ATK_RELATION_PARENT_WINDOW_OF] =
    Accessibility_RELATION_PARENT_WINDOW_OF;
  types[ATK_RELATION_DESCRIPTION_FOR] =
    Accessibility_RELATION_DESCRIPTION_FOR;
  types[ATK_RELATION_DESCRIBED_BY] = Accessibility_RELATION_DESCRIBED_BY;

  return TRUE;
}

static Accessibility_RelationType
spi_relation_type_from_atk_relation_type (AtkRelationType type)
{
  static gboolean is_initialized = FALSE;
  static Accessibility_RelationType
    spi_relation_type_table[ATK_RELATION_LAST_DEFINED];
  Accessibility_RelationType spi_type;

  if (!is_initialized)
    is_initialized = spi_init_relation_type_table (spi_relation_type_table);

  if (type > ATK_RELATION_NULL && type < ATK_RELATION_LAST_DEFINED)
    spi_type = spi_relation_type_table[type];
  else
    spi_type = Accessibility_RELATION_EXTENDED;
  return spi_type;
}

static DBusMessage *
impl_GetRelationSet (DBusConnection * bus,
                     DBusMessage * message, void *user_data)
{
  AtkObject *object = (AtkObject *) user_data;
  DBusMessage *reply;
  AtkRelationSet *set;
  DBusMessageIter iter, iter_array, iter_struct, iter_targets;
  gint count;
  gint i, j;

  g_return_val_if_fail (ATK_IS_OBJECT (user_data),
                        droute_not_yet_handled_error (message));
  reply = dbus_message_new_method_return (message);
  if (!reply)
    return NULL;
  set = atk_object_ref_relation_set (object);
  dbus_message_iter_init_append (reply, &iter);
  if (!dbus_message_iter_open_container
      (&iter, DBUS_TYPE_ARRAY, "(ua(so))", &iter_array))
    {
      goto oom;
    }
  count = atk_relation_set_get_n_relations (set);
  for (i = 0; i < count; i++)
    {
      AtkRelation *r = atk_relation_set_get_relation (set, i);
      AtkRelationType rt;
      GPtrArray *target;
      dbus_uint32_t type;
      if (!r)
        continue;
      rt = atk_relation_get_relation_type (r);
      type = spi_relation_type_from_atk_relation_type (rt);
      target = atk_relation_get_target (r);
      if (!dbus_message_iter_open_container
          (&iter_array, DBUS_TYPE_STRUCT, NULL, &iter_struct))
        {
          goto oom;
        }
      dbus_message_iter_append_basic (&iter_struct, DBUS_TYPE_UINT32, &type);
      if (!dbus_message_iter_open_container
          (&iter_struct, DBUS_TYPE_ARRAY, "(so)", &iter_targets))
        {
          goto oom;
        }
      for (j = 0; j < target->len; j++)
        {
          AtkObject *obj = target->pdata[j];
          char *path;
          if (!obj)
            continue;
          spi_object_append_reference (&iter_targets, obj);
        }
      dbus_message_iter_close_container (&iter_struct, &iter_targets);
      dbus_message_iter_close_container (&iter_array, &iter_struct);
    }
  dbus_message_iter_close_container (&iter, &iter_array);
oom:
  // TODO: handle out of memory */
  return reply;
}

static DBusMessage *
impl_GetRole (DBusConnection * bus, DBusMessage * message, void *user_data)
{
  AtkObject *object = (AtkObject *) user_data;
  gint role;
  dbus_uint32_t rv;
  DBusMessage *reply;

  g_return_val_if_fail (ATK_IS_OBJECT (user_data),
                        droute_not_yet_handled_error (message));
  role = atk_object_get_role (object);
  rv = spi_accessible_role_from_atk_role (role);
  reply = dbus_message_new_method_return (message);
  if (reply)
    {
      dbus_message_append_args (reply, DBUS_TYPE_UINT32, &rv,
                                DBUS_TYPE_INVALID);
    }
  return reply;
}

static char *
impl_get_role_str (void *datum)
{
  g_return_val_if_fail (ATK_IS_OBJECT (datum), g_strdup (""));
  return g_strdup_printf ("%d",
                          spi_accessible_role_from_atk_role
                          (atk_object_get_role ((AtkObject *) datum)));
}

static DBusMessage *
impl_GetRoleName (DBusConnection * bus,
                  DBusMessage * message, void *user_data)
{
  AtkObject *object = (AtkObject *) user_data;
  gint role;
  const char *role_name;
  DBusMessage *reply;

  g_return_val_if_fail (ATK_IS_OBJECT (user_data),
                        droute_not_yet_handled_error (message));
  role = atk_object_get_role (object);
  role_name = atk_role_get_name (role);
  if (!role_name)
    role_name = "";
  reply = dbus_message_new_method_return (message);
  if (reply)
    {
      dbus_message_append_args (reply, DBUS_TYPE_STRING, &role_name,
                                DBUS_TYPE_INVALID);
    }
  return reply;
}

static DBusMessage *
impl_GetLocalizedRoleName (DBusConnection * bus,
                           DBusMessage * message, void *user_data)
{
  AtkObject *object = (AtkObject *) user_data;
  gint role;
  const char *role_name;
  DBusMessage *reply;

  g_return_val_if_fail (ATK_IS_OBJECT (user_data),
                        droute_not_yet_handled_error (message));
  role = atk_object_get_role (object);
  role_name = atk_role_get_localized_name (role);
  if (!role_name)
    role_name = "";
  reply = dbus_message_new_method_return (message);
  if (reply)
    {
      dbus_message_append_args (reply, DBUS_TYPE_STRING, &role_name,
                                DBUS_TYPE_INVALID);
    }
  return reply;
}

static DBusMessage *
impl_GetState (DBusConnection * bus, DBusMessage * message, void *user_data)
{
  AtkObject *object = (AtkObject *) user_data;

  DBusMessage *reply = NULL;
  DBusMessageIter iter, iter_array;

  dbus_uint32_t states[2];

  guint count;

  g_return_val_if_fail (ATK_IS_OBJECT (user_data),
                        droute_not_yet_handled_error (message));

  reply = dbus_message_new_method_return (message);
  dbus_message_iter_init_append (reply, &iter);

  spi_atk_state_to_dbus_array (object, states);
  dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, "u", &iter_array);
  for (count = 0; count < 2; count++)
    {
      dbus_message_iter_append_basic (&iter_array, DBUS_TYPE_UINT32,
                                      &states[count]);
    }
  dbus_message_iter_close_container (&iter, &iter_array);
  return reply;
}

static DBusMessage *
impl_GetAttributes (DBusConnection * bus,
                    DBusMessage * message, void *user_data)
{
  AtkObject *object = (AtkObject *) user_data;
  AtkAttributeSet *attributes;
  DBusMessage *reply = NULL;
  DBusMessageIter iter;

  g_return_val_if_fail (ATK_IS_OBJECT (user_data),
                        droute_not_yet_handled_error (message));

  attributes = atk_object_get_attributes (object);

  reply = dbus_message_new_method_return (message);
  dbus_message_iter_init_append (reply, &iter);
  spi_object_append_attribute_set (&iter, attributes);

  atk_attribute_set_free (attributes);

  return reply;
}

static DBusMessage *
impl_GetApplication (DBusConnection * bus,
                     DBusMessage * message, void *user_data)
{
  AtkObject *root = atk_get_root ();
  return spi_object_return_reference (message, root);
}

static DBusMessage *
impl_GetInterfaces (DBusConnection * bus,
                    DBusMessage * message, void *user_data)
{
  AtkObject *object = (AtkObject *) user_data;
  gint role;
  const char *role_name;
  DBusMessage *reply;
  DBusMessageIter iter, iter_array;

  g_return_val_if_fail (ATK_IS_OBJECT (user_data),
                        droute_not_yet_handled_error (message));
  reply = dbus_message_new_method_return (message);
  if (reply)
    {
      dbus_message_iter_init_append (reply, &iter);
      dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, "s",
                                        &iter_array);
      spi_object_append_interfaces (&iter_array, object);
      dbus_message_iter_close_container (&iter, &iter_array);
    }
  return reply;
}

static DBusMessage *
impl_Embedded (DBusConnection *bus,
                    DBusMessage *message,
                    void *user_data)
{
  AtkObject *object = (AtkObject *) user_data;
  char *path;
  gchar *id;

  if (!dbus_message_get_args (message, NULL, DBUS_TYPE_STRING, &path, DBUS_TYPE_INVALID))
    {
      return droute_invalid_arguments_error (message);
    }
  id = g_object_get_data (G_OBJECT (object), "dbus-plug-parent");
  if (id)
    g_free (id);
  id = g_strconcat (dbus_message_get_sender (message), ":", path, NULL);
  g_object_set_data (G_OBJECT (object), "dbus-plug-parent", id);
  return dbus_message_new_method_return (message);
}

static DRouteMethod methods[] = {
  {impl_GetChildAtIndex, "GetChildAtIndex"},
  {impl_GetChildren, "GetChildren"},
  {impl_GetIndexInParent, "GetIndexInParent"},
  {impl_GetRelationSet, "GetRelationSet"},
  {impl_GetRole, "GetRole"},
  {impl_GetRoleName, "GetRoleName"},
  {impl_GetLocalizedRoleName, "GetLocalizedRoleName"},
  {impl_GetState, "GetState"},
  {impl_GetAttributes, "GetAttributes"},
  {impl_GetApplication, "GetApplication"},
  {impl_GetInterfaces, "GetInterfaces"},
  {impl_Embedded, "Embedded"},
  {NULL, NULL}
};

static DRouteProperty properties[] = {
  {impl_get_Name, impl_set_Name, "Name"},
  {impl_get_Description, impl_set_Description, "Description"},
  {impl_get_Parent, NULL, "Parent"},
  {impl_get_ChildCount, NULL, "ChildCount"},
  {NULL, NULL, NULL}
};

void
spi_initialize_accessible (DRoutePath * path)
{
  droute_path_add_interface (path,
                             SPI_DBUS_INTERFACE_ACCESSIBLE,
                             methods, properties);
};