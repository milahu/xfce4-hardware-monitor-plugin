/* An abstract base class for canvas-based views.
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

#ifndef CANVAS_VIEW_HPP
#define CANVAS_VIEW_HPP

#include <list>
#include <vector>
#include <memory>

#include <libgnomecanvasmm/canvas.h>
#include <libgnomecanvasmm/text.h>
#include <glibmm/ustring.h>
#include <gtkmm/frame.h>

#include "view.hpp"


class Canvas;

class CanvasView: public View, public sigc::trackable
{
public:

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

  CanvasView(bool keeps_history);
  ~CanvasView();

  /* Used to locate monitor type of interest in monitor_maxes during
   * visualisation draw loop */
  typedef std::map<Glib::ustring, std::pair<int, int>>::iterator
      mon_type_iterator;

  static const Glib::ustring text_overlay_position_to_string(
      TextOverlayPosition position);

  static int const draw_interval;
  // for animation, number of drawings to break an update into
  static int const draw_iterations;

protected:
  virtual void do_display();
  virtual void do_update();
  virtual void do_set_background(unsigned int color);
  virtual void do_unset_background();

  /* Included in the header as other compilation units need access to the
   * definition in order to instantiate the relevant template function
   * TODO: When I officially move to C++11, implement 'alias templates' as a
   * form of typedefs that work with template declarations */
  template <typename T>
  std::list<std::pair<T*, double>> process_mon_maxes_text_overlay(
      typename std::list<T*> graph_elements);

  int width() const;
  int height() const;
  void resize_canvas();   // resize canvas according to width and height

  int size;     // in pixels, width when vertical, else height

  std::auto_ptr<Gnome::Canvas::Canvas> canvas;

  sigc::connection draw_timer;
  
private:
  bool draw_loop();
  virtual void do_draw_loop() = 0;

  void text_overlay_calc_position(int &x, int &y, TextOverlayPosition position);

  Gnome::Canvas::Text *text_overlay;

  // Text overlay format string substitution codes
  static const Glib::ustring monitor_full;
  static const Glib::ustring monitor_compact;
  static const Glib::ustring graph_max_full;
  static const Glib::ustring graph_max_compact;
};

#endif
