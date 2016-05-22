/* Helper functions.
 *
 * Copyright (c) 2003, 04 Ole Laursen.
 * Copyright (c) 2013, 2016 OmegaPhil (OmegaPhil@startmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 */

#include <config.h>

#include "i18n.hpp"

#include "gui-helpers.hpp"

// Spun this out into a cpp file as the i18n include was clashing

Glib::RefPtr<Gtk::Builder> get_builder_xml(std::vector<Glib::ustring> objects)
{
  /* Now we are forced to use top-level widgets this requires a list of objects
   * to instantiate... */
  try
  {
    return Gtk::Builder::create_from_file(HARDWARE_MONITOR_GLADEDIR
                                          "ui.glade", objects);
  }
  catch (Gtk::BuilderError &error)
  {

    // Including true error number so that it can be looked up
    Glib::ustring error_string = Glib::ustring::compose(
          _("Unable to load the dialog '%1' due to the following GtkBuilder error:"
            "\n\n%2 (%3)"), objects[0], error.what(), error.code());
    fatal_error(error_string);
    return Glib::RefPtr<Gtk::Builder>();
  }
}

