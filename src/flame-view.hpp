/* A view which displays steady flames, the sizes of which are determined by the
 * monitor values.
 *
 * Copyright (c) 2003, 04 Ole Laursen.
 * Copyright (c) 2013, 2015-2018 OmegaPhil (OmegaPhil@startmail.com)
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

#ifndef FLAME_VIEW_HPP
#define FLAME_VIEW_HPP

#include <list>
#include <vector>
#include <memory>

#include <libgnomecanvasmm/canvas.h>
#include <libgnomecanvasmm/pixbuf.h>

#include <glibmm/ustring.h>

#include "canvas-view.hpp"


//
// class Flame - represents a flame layer
//

class Flame
{
public:
  Flame(Monitor *monitor_, unsigned int color_);

  void burn(double overall_max);
  double get_max_value();
  void update(Gnome::Canvas::Canvas &canvas, int width, int height);

  Monitor *monitor;

private:
  std::auto_ptr<Gnome::Canvas::Pixbuf> flame;

  double value, max;

  std::vector<unsigned char> fuel;
  int next_refuel;
  int cooling;      // cooling factor

  void recompute_fuel(double overall_max);
  unsigned int color;
};


class FlameView: public CanvasView
{
public:
  FlameView(Plugin &plugin_);
  ~FlameView();
  
private:
  virtual void do_update();
  virtual void do_attach(Monitor *monitor);
  virtual void do_detach(Monitor *monitor);
  virtual void do_draw_loop();

  // must be destroyed before the canvas
  typedef std::list<Flame *> flame_sequence;
  typedef flame_sequence::iterator flame_iterator;
  flame_sequence flames;
};

#endif
