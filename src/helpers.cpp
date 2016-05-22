/* Helper functions.
 *
 * Copyright (c) 2003 Ole Laursen.
 * Copyright (c) 2013, 2015 OmegaPhil (OmegaPhil@startmail.com)
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

#include <gtkmm/messagedialog.h>

#include "helpers.hpp"
#include "i18n.hpp"


void fatal_error(const Glib::ustring &msg)
{
  Gtk::MessageDialog d(msg, Gtk::MESSAGE_ERROR);

  d.set_modal();
  d.set_title(_("Fatal Error"));

  d.run();
  
  exit(1);
}

// See header note for why buttons isn't a Gtk type
/*
int warning_dialog(const Glib::ustring &msg, const Glib::ustring &title,
                   Glib::RefPtr<Gdk::Pixbuf> icon, const int buttons)
{
  Gtk::ButtonsType buttons_type = static_cast<Gtk::ButtonsType>(buttons);

  Gtk::MessageDialog d(msg, false, Gtk::MESSAGE_WARNING, buttons_type);

  d.set_modal();
  d.set_title(title);
  d.set_icon(icon);

  return d.run();
}
*/

// Why does std::string or Glib::ustring not have a replace all function??
void find_and_replace(Glib::ustring &source, const Glib::ustring &to_replace,
                      const Glib::ustring &replace_with)
{
  Glib::ustring::size_type pos = 0;
  while ((pos = source.find(to_replace, pos)) != Glib::ustring::npos)
  {
    source = source.replace(pos, to_replace.length(), replace_with);
    pos += replace_with.length();
  }
}

Glib::ustring truncate_string(Glib::ustring s, unsigned int n)
{
  // for when a string needs to be truncated
  Glib::ustring ellipsis = "...";

  if (s.length() > n && n - ellipsis.length() > 0)
    s.replace(n - ellipsis.length(), Glib::ustring::npos, ellipsis);
  
  return s;
}

