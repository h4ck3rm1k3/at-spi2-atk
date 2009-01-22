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

#include <stdlib.h>
#include <string.h>

#include "droute-variant.h"

/*---------------------------------------------------------------------------*/

dbus_bool_t
droute_return_v_int32 (DBusMessageIter *iter, dbus_int32_t val)
{
    DBusMessageIter sub;

    if (!dbus_message_iter_open_container
        (iter, DBUS_TYPE_VARIANT, DBUS_TYPE_INT32_AS_STRING, &sub))
      {
        return FALSE;
      }
    dbus_message_iter_append_basic (&sub, DBUS_TYPE_INT32, &val);
    dbus_message_iter_close_container (iter, &sub);
    return TRUE;
}

dbus_bool_t
droute_return_v_double (DBusMessageIter *iter, double val)
{
    DBusMessageIter sub;

    if (!dbus_message_iter_open_container
        (iter, DBUS_TYPE_VARIANT, DBUS_TYPE_DOUBLE_AS_STRING, &sub))
      {
        return FALSE;
      }
    dbus_message_iter_append_basic (&sub, DBUS_TYPE_DOUBLE, &val);
    dbus_message_iter_close_container (iter, &sub);
    return TRUE;
}

dbus_bool_t
droute_return_v_string (DBusMessageIter *iter, const char *val)
{
    DBusMessageIter sub;

    if (!val)
      val = "";
    if (!dbus_message_iter_open_container
        (iter, DBUS_TYPE_VARIANT, DBUS_TYPE_STRING_AS_STRING, &sub))
      {
        return FALSE;
      }
    dbus_message_iter_append_basic (&sub, DBUS_TYPE_STRING, &val);
    dbus_message_iter_close_container (iter, &sub);
    return TRUE;
}

dbus_bool_t
droute_return_v_object (DBusMessageIter *iter, const char *path)
{
    DBusMessageIter sub;

    if (!dbus_message_iter_open_container
        (iter, DBUS_TYPE_VARIANT, DBUS_TYPE_OBJECT_PATH_AS_STRING, &sub))
      {
        return FALSE;
      }
    dbus_message_iter_append_basic (&sub, DBUS_TYPE_OBJECT_PATH, &path);
    dbus_message_iter_close_container (iter, &sub);
    return TRUE;
}

/*---------------------------------------------------------------------------*/

dbus_int32_t
droute_get_v_int32 (DBusMessageIter *iter)
{
    DBusMessageIter sub;
    dbus_int32_t rv;

    // TODO- ensure we have the correct type
    dbus_message_iter_recurse (iter, &sub);
    dbus_message_iter_get_basic (&sub, &rv);
    return rv;
}

const char *
droute_get_v_string (DBusMessageIter *iter)
{
    DBusMessageIter sub;
    char *rv;

    // TODO- ensure we have the correct type
    dbus_message_iter_recurse (iter, &sub);
    dbus_message_iter_get_basic (&sub, &rv);
    return rv;
}

/*END------------------------------------------------------------------------*/