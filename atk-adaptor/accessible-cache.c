/*
 * AT-SPI - Assistive Technology Service Provider Interface
 * (Gnome Accessibility Project; http://developer.gnome.org/projects/gap)
 *
 * Copyright 2009, 2010 Codethink Ltd.
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

#include "accessible-cache.h"
#include "accessible-register.h"
#include "bridge.h"

SpiCache *spi_global_cache = NULL;

static gboolean
child_added_listener (GSignalInvocationHint * signal_hint,
                      guint n_param_values,
                      const GValue * param_values, gpointer data);

static void
toplevel_added_listener (AtkObject * accessible,
                         guint index, AtkObject * child);

static void
remove_object (gpointer data, GObject * gobj);

static void
add_object (SpiCache * cache, GObject * gobj);

static void
add_subtree (SpiCache *cache, AtkObject * accessible);

/*---------------------------------------------------------------------------*/

static void
spi_cache_finalize (GObject * object);

static void
spi_cache_dispose (GObject * object);

/*---------------------------------------------------------------------------*/

enum
{
  OBJECT_ADDED,
  OBJECT_REMOVED,
  LAST_SIGNAL
};
static guint cache_signals[LAST_SIGNAL] = { 0 };

/*---------------------------------------------------------------------------*/

G_DEFINE_TYPE (SpiCache, spi_cache, G_TYPE_OBJECT)

static void spi_cache_class_init (SpiCacheClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  spi_cache_parent_class = g_type_class_ref (G_TYPE_OBJECT);

  object_class->finalize = spi_cache_finalize;
  object_class->dispose = spi_cache_dispose;

  cache_signals [OBJECT_ADDED] = \
      g_signal_new ("object-added",
                    SPI_CACHE_TYPE,
                    G_SIGNAL_ACTION,
                    0,
                    NULL,
                    NULL,
                    g_cclosure_marshal_VOID__OBJECT,
                    G_TYPE_NONE,
                    1,
                    G_TYPE_OBJECT);

  cache_signals [OBJECT_REMOVED] = \
      g_signal_new ("object-removed",
                    SPI_CACHE_TYPE,
                    G_SIGNAL_ACTION,
                    0,
                    NULL,
                    NULL,
                    g_cclosure_marshal_VOID__OBJECT,
                    G_TYPE_NONE,
                    1,
                    G_TYPE_OBJECT);
}

static void
spi_cache_init (SpiCache * cache)
{
  cache->objects = g_hash_table_new (g_direct_hash, g_direct_equal);

#ifdef SPI_ATK_DEBUG
  if (g_thread_supported ())
    g_message ("AT-SPI: Threads enabled");

  g_debug ("AT-SPI: Initial Atk tree regisration");
#endif

  g_signal_connect (spi_global_register,
                    "object-deregistered",
                    (GCallback) remove_object, cache);

  add_subtree (cache, spi_global_app_data->root);

  atk_add_global_event_listener (child_added_listener,
                                 "Gtk:AtkObject:children-changed");

  g_signal_connect (G_OBJECT (spi_global_app_data->root),
                    "children-changed::add",
                    (GCallback) toplevel_added_listener, NULL);
}

static void
spi_cache_finalize (GObject * object)
{
  SpiCache *cache = SPI_CACHE (object);

  g_free (cache->objects);

  G_OBJECT_CLASS (spi_cache_parent_class)->finalize (object);
}

static void
spi_cache_dispose (GObject * object)
{
  SpiCache *cache = SPI_CACHE (object);

  G_OBJECT_CLASS (spi_cache_parent_class)->dispose (object);
}

/*---------------------------------------------------------------------------*/

static void
remove_object (gpointer data, GObject * gobj)
{
  SpiCache *cache = SPI_CACHE (data);
  
  if (spi_cache_in (cache, gobj))
    {
      g_signal_emit (cache, cache_signals [OBJECT_REMOVED], 0, gobj);
      g_hash_table_remove (cache->objects, gobj);
    }
}

static void
add_object (SpiCache * cache, GObject * gobj)
{
  g_return_if_fail (G_IS_OBJECT (gobj));

  g_hash_table_insert (cache->objects, gobj, NULL);

  g_signal_emit (cache, cache_signals [OBJECT_ADDED], 0, gobj);
}

/*---------------------------------------------------------------------------*/

static GStaticRecMutex cache_mutex        = G_STATIC_REC_MUTEX_INIT;
static GStaticMutex recursion_check_guard = G_STATIC_MUTEX_INIT;

static gboolean recursion_check = FALSE;

static gboolean
recursion_check_and_set ()
{
  gboolean ret;
  g_static_mutex_lock (&recursion_check_guard);
  ret = recursion_check;
  recursion_check = TRUE;
  g_static_mutex_unlock (&recursion_check_guard);
  return ret;
}

static void
recursion_check_unset ()
{
  g_static_mutex_lock (&recursion_check_guard);
  recursion_check = FALSE;
  g_static_mutex_unlock (&recursion_check_guard);
}

/*---------------------------------------------------------------------------*/

static void
append_children (AtkObject * accessible, GQueue * traversal)
{
  AtkObject *current;
  guint i;
  gint count = atk_object_get_n_accessible_children (accessible);

  if (count < 0)
    count = 0;
  for (i = 0; i < count; i++)
    {
      current = atk_object_ref_accessible_child (accessible, i);
      if (current)
        {
          g_queue_push_tail (traversal, current);
        }
    }
}

/*
 * Adds a subtree of accessible objects
 * to the cache at the accessible object provided.
 *
 * The leaf nodes do not have their children
 * registered. A node is considered a leaf
 * if it has the state "manages-descendants"
 * or if it has already been registered.
 */
static void
add_subtree (SpiCache *cache, AtkObject * accessible)
{
  AtkObject *current;
  GQueue *traversal;
  GQueue *to_add;

  g_return_if_fail (ATK_IS_OBJECT (accessible));

  traversal = g_queue_new ();
  to_add = g_queue_new ();

  g_object_ref (accessible);
  g_queue_push_tail (traversal, accessible);

  while (!g_queue_is_empty (traversal))
    {
      AtkStateSet *set;
      
      current = g_queue_pop_head (traversal);
      set = atk_object_ref_state_set (current);

      if (!atk_state_set_contains_state (set, ATK_STATE_TRANSIENT))
        {
	  g_queue_push_tail (to_add, current);
          if (!spi_cache_in (cache, G_OBJECT (current)) &&
              !atk_state_set_contains_state  (set, ATK_STATE_MANAGES_DESCENDANTS))
            {
#ifdef SPI_ATK_DEBUG
              g_debug ("REG  - %s - %d - %s", atk_object_get_name (current),
                       atk_object_get_role (current),
                       atk_dbus_object_to_path (current));
#endif
              append_children (current, traversal);
            }
        }

      g_object_unref (set);
    }

  while (!g_queue_is_empty (to_add))
    {
      current = g_queue_pop_head (to_add);
      add_object (cache, G_OBJECT(current));
      g_object_unref (G_OBJECT (current));
    }

  g_queue_free (traversal);
  g_queue_free (to_add);
}

/*---------------------------------------------------------------------------*/

static gboolean
child_added_listener (GSignalInvocationHint * signal_hint,
                      guint n_param_values,
                      const GValue * param_values, gpointer data)
{
  SpiCache *cache = spi_global_cache;

  AtkObject *accessible;
  AtkObject *child;

  const gchar *detail = NULL;

  g_static_rec_mutex_lock (&cache_mutex);

  /* 
   * Ensure that only accessibles already in the cache
   * have their signals processed.
   */
  accessible = ATK_OBJECT (g_value_get_object (&param_values[0]));
  g_return_val_if_fail (ATK_IS_OBJECT (accessible), TRUE);

  if (spi_cache_in (cache, G_OBJECT(accessible)))
    {
#ifdef SPI_ATK_DEBUG
      if (recursion_check_and_set ())
        g_warning ("AT-SPI: Recursive use of cache module");

      g_debug ("AT-SPI: Tree update listener");
#endif
      if (signal_hint->detail)
        detail = g_quark_to_string (signal_hint->detail);

      if (!strcmp (detail, "add"))
        {
          gpointer child;
          int index = g_value_get_uint (param_values + 1);
          child = g_value_get_pointer (param_values + 2);

          if (!ATK_IS_OBJECT (child))
           {
             child = atk_object_ref_accessible_child (accessible, index);
           }
          add_subtree (cache, child);
        }
#ifdef SPI_ATK_DEBUG
      recursion_check_unset ();
#endif
    }

  g_static_rec_mutex_unlock (&cache_mutex);

  return TRUE;
}

/*---------------------------------------------------------------------------*/

static void
toplevel_added_listener (AtkObject * accessible,
                         guint index, AtkObject * child)
{
  SpiCache *cache = spi_global_cache;

  g_static_rec_mutex_lock (&cache_mutex);

  g_return_if_fail (ATK_IS_OBJECT (accessible));

  if (spi_cache_in (cache, G_OBJECT(accessible)))
    {
#ifdef SPI_ATK_DEBUG
      if (recursion_check_and_set ())
        g_warning ("AT-SPI: Recursive use of registration module");

      g_debug ("AT-SPI: Toplevel added listener");
#endif
      if (!ATK_IS_OBJECT (child))
        {
          child = atk_object_ref_accessible_child (accessible, index);
        }
      add_subtree (cache, child);
#ifdef SPI_ATK_DEBUG
      recursion_check_unset ();
#endif
    }

  g_static_rec_mutex_unlock (&cache_mutex);
}

/*---------------------------------------------------------------------------*/

void
spi_cache_foreach (SpiCache * cache, GHFunc func, gpointer data)
{
  g_hash_table_foreach (cache->objects, func, data);
}

gboolean
spi_cache_in (SpiCache * cache, GObject * object)
{
  if (g_hash_table_lookup_extended (cache->objects,
                                    object,
                                    NULL,
                                    NULL))
    return TRUE;
  else
    return FALSE;
}

/*END------------------------------------------------------------------------*/