#include <string.h>
#include <libbonobo.h>
#include "../cspi-lowlevel.h"

void
cspi_dup_ref (CORBA_Object object)
{
  bonobo_object_dup_ref (object, NULL);
}

void
cspi_release_unref (CORBA_Object object)
{
  bonobo_object_release_unref (object, NULL);
}

SPIBoolean
cspi_check_ev (const char *error_string)
{
  CORBA_Environment *ev = cspi_ev ();

  if (ev->_major != CORBA_NO_EXCEPTION)
    {
      char *err;

      err = bonobo_exception_get_text (ev);

      fprintf (stderr, "Warning: AT-SPI error: %s: %s\n",
	       error_string, err);

      g_free (err);

      CORBA_exception_free (ev);

      return FALSE;
    }
  else
    {
      return TRUE;
    }
}

char *
cspi_exception_get_text (void)
{
  char *ret, *txt;

  txt = bonobo_exception_get_text (cspi_ev ());
  ret = strdup (txt);
  g_free (txt);

  return ret;
}

CORBA_Object
cspi_init (void)
{
  char *obj_id;
  CORBA_Object registry;
  CORBA_Environment ev;

  if (!bonobo_init (0, NULL))
    {
      g_error ("Could not initialize Bonobo");
    }

  obj_id = "OAFIID:Accessibility_Registry:proto0.1";

  CORBA_exception_init (&ev);

  registry = bonobo_activation_activate_from_id (
    obj_id, 0, NULL, &ev);

  if (ev._major != CORBA_NO_EXCEPTION)
    {
      g_error ("AT-SPI error: during registry activation: %s\n",
	       bonobo_exception_get_text (&ev));
    }

  if (registry == CORBA_OBJECT_NIL)
    {
      g_error ("Could not locate registry");
    }

  bonobo_activate ();

  return registry;
}

void
cspi_main (void)
{
  bonobo_main ();
}

void
cspi_main_quit (void)
{
  bonobo_main_quit ();
}