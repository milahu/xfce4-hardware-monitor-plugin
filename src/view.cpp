/* Implementation of view base class.
 *
 * Copyright (c) 2003, 04 Ole Laursen.
 * Copyright (c) 2013, 2016-2018 OmegaPhil (OmegaPhil@startmail.com)
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

#include "view.hpp"

View::View(bool keeps_history_, Plugin &plugin_)
  : keeps_history(keeps_history_), plugin(plugin_)
{
}

View::View::~View()
{
}

void View::display()
{
  do_display();
}

void View::update()
{
  do_update();
}

void View::attach(Monitor *monitor)
{
  do_attach(monitor);
}

void View::detach(Monitor *monitor)
{
  do_detach(monitor);
}

void View::set_background(unsigned int color)
{
  do_set_background(color);
}

void View::unset_background()
{
  do_unset_background();
}
