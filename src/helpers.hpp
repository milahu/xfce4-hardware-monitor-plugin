/* Helper functions.
 *
 * Copyright (c) 2003 Ole Laursen.
 * Copyright (c) 2013, 2015-2016 OmegaPhil (OmegaPhil@startmail.com)
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HELPERS_HPP
#define HELPERS_HPP

//#include <gdkmm/pixbuf.h>  // For icon in warning_dialog
#include <glibmm/ustring.h>

/* Can't include gtkmm/messagedialog.h here as it causes X11 header to be
 * included before gtkmm?? */
//#define LOCAL_GTK_BUTTONS_OK 1

// from www.boost.org - derivation from this class makes the derived class
// noncopyable
class noncopyable
{
protected:
  noncopyable() {}
  ~noncopyable() {}
private:
  noncopyable(const noncopyable&);
  const noncopyable& operator=(const noncopyable&);
};

void fatal_error(const Glib::ustring &msg);
void find_and_replace(Glib::ustring &source, const Glib::ustring &to_replace,
                      const Glib::ustring &replace_with);

/* Attempting to host the warning_dialog code here has failed completely -
 * constant bullshit include errors either not allowing namespace items to be
 * defined, or some X11/gtkmm clash again - this is all needed to allow for
 * the parameters passed here */
/*int warning_dialog(const Glib::ustring &msg, const Glib::ustring &title,
                   Glib::RefPtr<Gdk::Pixbuf> icon,
                   const int buttons = LOCAL_GTK_BUTTONS_OK);*/

#endif
