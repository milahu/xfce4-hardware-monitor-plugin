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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GUI_HELPERS_HPP
#define GUI_HELPERS_HPP

#include <glibmm/ustring.h>
#include <gtkmm/builder.h>

#include "helpers.hpp"

// Helper for loading a GtkBuilder XML file
Glib::RefPtr<Gtk::Builder> get_builder_xml(std::vector<Glib::ustring> objects);

#endif
