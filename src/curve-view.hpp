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

#include <list>
#include <vector>
#include <memory>

#include <libgnomecanvasmm/canvas.h>
#include <libgnomecanvasmm/text.h>
#include <glibmm/ustring.h>

#include "canvas-view.hpp"


class Curve;

class CurveView: public CanvasView
{
public:
  CurveView();
  ~CurveView();
  
  static int const pixels_per_sample;

  enum TextOverlayPosition {
     top_left,
     top_center,
     top_right,
     center,
     bottom_left,
     bottom_center,
     bottom_right,
     NUM_TEXT_OVERLAY_POSITIONS
  };

  static const Glib::ustring text_overlay_position_to_string(
      TextOverlayPosition position);

private:
  virtual void do_update();
  virtual void do_attach(Monitor *monitor);
  virtual void do_detach(Monitor *monitor);
  virtual void do_draw_loop();

  void text_overlay_calc_position(int &x, int &y, TextOverlayPosition position);

  // Must be destroyed before the canvas
  typedef std::list<Curve *> curve_sequence;
  typedef curve_sequence::iterator curve_iterator;
  curve_sequence curves;

  Gnome::Canvas::Text *text_overlay;

  // Text overlay format string substitution codes
  static const Glib::ustring monitor_full;
  static const Glib::ustring monitor_compact;
  static const Glib::ustring graph_max_full;
  static const Glib::ustring graph_max_compact;
};

#endif
