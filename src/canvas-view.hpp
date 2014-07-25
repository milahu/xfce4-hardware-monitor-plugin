/* An abstract base class for canvas-based views.
 *
 * Copyright (c) 2003, 04 Ole Laursen.
 * Copyright (c) 2013 OmegaPhil (OmegaPhil00@startmail.com)
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

#ifndef CANVAS_VIEW_HPP
#define CANVAS_VIEW_HPP

#include <list>
#include <vector>
#include <memory>

#include <libgnomecanvasmm/canvas.h>
#include <glibmm/ustring.h>
#include <gtkmm/frame.h>

#include "view.hpp"


class Canvas;

class CanvasView: public View, public sigc::trackable
{
public:
  CanvasView(bool keeps_history);
  ~CanvasView();

  static int const draw_interval;
  // for animation, number of drawings to break an update into
  static int const draw_iterations;
  
protected:
  virtual void do_display();
  virtual void do_update();
  virtual void do_set_background(unsigned int color);
  virtual void do_unset_background();

  int width() const;
  int height() const;
  void resize_canvas();   // resize canvas according to width and height

  int size;     // in pixels, width when vertical, else height

  std::auto_ptr<Gnome::Canvas::Canvas> canvas;

  sigc::connection draw_timer;
  
private:
  bool draw_loop();
  virtual void do_draw_loop() = 0;
};

#endif
