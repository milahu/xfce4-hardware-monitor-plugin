/* A view which displays a bar plot.
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

#ifndef BAR_VIEW_HPP
#define BAR_VIEW_HPP

#include <list>
#include <vector>
#include <memory>

#include <libgnomecanvasmm/canvas.h>
#include <libgnomecanvasmm/rect.h>

#include <glibmm/ustring.h>

#include "canvas-view.hpp"

//
// class Bar - represents a single bar graph
//

class Bar
{
public:
  Bar(Monitor *monitor_, unsigned int fill_color_, bool horizontal_ = false);
  ~Bar();

  void draw(Gnome::Canvas::Canvas &canvas, int width, int height, int no,
            int total, double time_offset, double max);
  double get_max_value();
  void update();

  Monitor *monitor;

private:
  typedef std::vector<Gnome::Canvas::Rect *> box_sequence;
  box_sequence boxes;

  double old_value, new_value;
  bool horizontal;
  unsigned int fill_color;
};

class BarView: public CanvasView
{
public:
  BarView(Plugin &plugin_, bool horizontal_ = true);
  ~BarView();
  virtual bool is_horizontal();
  
private:
  virtual void do_update();
  virtual void do_attach(Monitor *monitor);
  virtual void do_detach(Monitor *monitor);
  virtual void do_draw_loop();

  // must be destroyed before the canvas
  typedef std::list<Bar *> bar_sequence;
  typedef bar_sequence::iterator bar_iterator;
  bar_sequence bars;
  
  int draws_since_update;
  bool horizontal;
};

#endif
