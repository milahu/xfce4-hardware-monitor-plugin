/* A view which displays a (time, value) curve plot.
 *
 * Copyright (c) 2003, 04 Ole Laursen.
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

#ifndef CURVE_VIEW_HPP
#define CURVE_VIEW_HPP

#include <memory>

#include <libgnomecanvasmm/canvas.h>
#include <libgnomecanvasmm/line.h>
#include <libgnomecanvasmm/text.h>
#include <glibmm/ustring.h>

#include "canvas-view.hpp"
#include "value-history.hpp"


//
// class Curve - represents a line curve
//

class Curve
{
public:
  Curve(Monitor *monitor, unsigned int color);

  void update(unsigned int max_samples);  // Gather info from monitor
  void draw(Gnome::Canvas::Canvas &canvas,  // Redraw curve on canvas
      int width, int height, double max);
  double get_max_value();  // Used to get overall max across curves

  Monitor *monitor;

private:
  std::auto_ptr<Gnome::Canvas::Line> line;

  ValueHistory value_history;
  int remaining_draws;
  unsigned int color;
};


class CurveView: public CanvasView
{
public:
  CurveView();
  ~CurveView();
  
  static int const pixels_per_sample;

private:
  virtual void do_update();
  virtual void do_attach(Monitor *monitor);
  virtual void do_detach(Monitor *monitor);
  virtual void do_draw_loop();

  // Must be destroyed before the canvas
  typedef std::list<Curve *> curve_sequence;
  typedef curve_sequence::iterator curve_iterator;
  curve_sequence curves;
};

#endif
