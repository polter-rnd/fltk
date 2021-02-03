//
// Rectangle drawing routines for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2018 by Bill Spitzak and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file.  If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//
// Please see the following page on how to report bugs and issues:
//
//     https://www.fltk.org/bugs.php
//


/**
 \file Fl_GDI_Graphics_Driver_rect.cxx
 \brief Windows GDI specific line and polygon drawing with integer coordinates.
 */

#include <config.h>
#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include <FL/platform.H>

#include "Fl_GDI_Graphics_Driver.H"

#if USE_GDIPLUS

void Fl_GDIplus_Graphics_Driver::point(int x, int y) {
  float s = scale();
  float point_size = (s >= 1 ? int(s)/s : 1/s);
  graphics_->FillRectangle(brush_, int(x*s)/s, int(y*s)/s, point_size, point_size);
}

void Fl_GDIplus_Graphics_Driver::overlay_rect(int x, int y, int w , int h) {
  float s = scale();
  pen_->SetWidth(1/s); // make pen have a one-pixel width
  graphics_->DrawRectangle(pen_, Gdiplus::RectF((int(x*s)+0.5f)/s, (int(y*s)+0.5f)/s, (int((x+w-1)*s) - int(x*s))/s, (int((y+h-1)*s) - int(y*s))/s));
  pen_->SetWidth(line_width_);
}

void Fl_GDIplus_Graphics_Driver::rect(int x, int y, int w, int h)
{
  if (w > 0 && h > 0) {
    float s = scale();
    if (s != int(s) && line_width_ == 1) {
      int lwidth = int((y+1)*s) - int(y*s);
      pen_->SetWidth(lwidth/s);
      graphics_->DrawLine(pen_, int(x*s)/s, (int(y*s)+lwidth/2.)/s, (int((x+w)*s) - 0.5)/s, (int(y*s)+lwidth/2.)/s);
      lwidth = int((x+w)*s) - int((x+w-1)*s);
      pen_->SetWidth(lwidth/s);
      graphics_->DrawLine(pen_, (int((x+w-1)*s)+lwidth/2.)/s, int(y*s)/s, (int((x+w-1)*s)+lwidth/2.)/s, (int((y+h)*s) - 0.5)/s);
      lwidth = int((y+h)*s) - int((y+h-1)*s);
      pen_->SetWidth(lwidth/s);
      graphics_->DrawLine(pen_, int(x*s)/s, (int((y+h-1)*s)+lwidth/2.)/s, (int((x+w)*s) - 0.5)/s, (int((y+h-1)*s)+lwidth/2.)/s);
      lwidth = int((x+1)*s) - int((x)*s);
      pen_->SetWidth(lwidth/s);
      graphics_->DrawLine(pen_, (int(x*s)+lwidth/2.)/s, int(y*s)/s, (int(x*s)+lwidth/2.)/s, (int((y+h)*s) - 0.5)/s);
      pen_->SetWidth(1);
    }
    else {
      graphics_->DrawRectangle(pen_, Gdiplus::RectF(x+0.5f, y+0.5f, w-1.f, h-1.f));
    }
  }
}

void Fl_GDIplus_Graphics_Driver::focus_rect(int x, int y, int w, int h) {
  pen_->SetDashStyle(Gdiplus::DashStyleDot);
  rect(x, y, w, h);
  pen_->SetDashStyle(Gdiplus::DashStyleSolid);
}

void Fl_GDIplus_Graphics_Driver::rectf(int x, int y, int w, int h) {
  if (w<=0 || h<=0) return;
  float s = scale();
  Gdiplus::RectF *rect;
  if (s != int(s)) rect = new Gdiplus::RectF( int(x*s)/s, int(y*s)/s,
                        (int((x+w)*s) - int(x*s))/s, (int((y+h)*s) - int(y*s))/s);
  else rect = new Gdiplus::RectF(x, y, w, h);
  graphics_->FillRectangle(brush_, *rect);
  delete rect;
}

void Fl_GDIplus_Graphics_Driver::xyline(int x, int y, int x1) {
  float s = scale();
  int xx = (x < x1 ? x : x1);
  int xx1 = (x < x1 ? x1 : x);
  if (s != int(s) && line_width_ == 1) {
    int lwidth = int((y+1)*s) - int(y*s);
    pen_->SetWidth(lwidth/s);
    graphics_->DrawLine(pen_, int(xx*s)/s, (int(y*s)+lwidth/2.)/s, (int((xx1+1)*s) - 0.5)/s, (int(y*s)+lwidth/2.)/s);
    pen_->SetWidth(line_width_);
  } else {
    graphics_->DrawLine(pen_, (float)xx, y+1-line_width_/2.f, xx1+0.75f, y+1-line_width_/2.f);
  }
}

void Fl_GDIplus_Graphics_Driver::xyline(int x, int y, int x1, int y2) {
  xyline(x, y, x1);
  yxline(x1, y, y2);
}

void Fl_GDIplus_Graphics_Driver::xyline(int x, int y, int x1, int y2, int x3) {
  xyline(x, y, x1);
  yxline(x1, y, y2);
  xyline(x1, y2, x3);
}

void Fl_GDIplus_Graphics_Driver::yxline(int x, int y, int y1) {
  float s = scale();
  int yy = (y < y1 ? y : y1);
  int yy1 = (y < y1 ? y1 : y);
  if (s != int(s) && line_width_ == 1) {
    int lwidth = int((x+1)*s) - int(x*s);
    pen_->SetWidth(lwidth/s);
    graphics_->DrawLine(pen_, (int(x*s)+lwidth/2.)/s, int(yy*s)/s, (int(x*s)+lwidth/2.)/s, (int((yy1+1)*s) - 0.5)/s);
    pen_->SetWidth(line_width_);
  } else {
    graphics_->DrawLine(pen_, x+1-line_width_/2.f, (float)yy, x+1-line_width_/2.f, yy1+0.75f);
  }
}

void Fl_GDIplus_Graphics_Driver::yxline(int x, int y, int y1, int x2) {
  yxline(x, y, y1);
  xyline(x, y1, x2);
}

void Fl_GDIplus_Graphics_Driver::yxline(int x, int y, int y1, int x2, int y3) {
  yxline(x, y, y1);
  xyline(x, y1, x2);
  yxline(x2, y1, y3);
}

void Fl_GDIplus_Graphics_Driver::line(int x, int y, int x1, int y1) {
  graphics_->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
  graphics_->DrawLine(pen_, x, y, x1, y1);
  graphics_->SetSmoothingMode(Gdiplus::SmoothingModeDefault);
}

void Fl_GDIplus_Graphics_Driver::line(int x, int y, int x1, int y1, int x2, int y2) {
  Gdiplus::GraphicsPath path;
  Gdiplus::Point gdi2_p[3] = {Gdiplus::Point(x, y), Gdiplus::Point(x1, y1), Gdiplus::Point(x2, y2)};
  path.AddLines(gdi2_p, 3);
  graphics_->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
  graphics_->DrawPath(pen_, &path);
  graphics_->SetSmoothingMode(Gdiplus::SmoothingModeDefault);
}

void Fl_GDIplus_Graphics_Driver::loop(int x0, int y0, int x1, int y1, int x2, int y2) {
  Gdiplus::GraphicsPath path;
  Gdiplus::Point gdi2_p[3] = {Gdiplus::Point(x0, y0), Gdiplus::Point(x1, y1), Gdiplus::Point(x2, y2)};
  path.AddLines(gdi2_p, 3);
  path.CloseFigure();
  graphics_->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
  graphics_->DrawPath(pen_, &path);
  graphics_->SetSmoothingMode(Gdiplus::SmoothingModeDefault);
}

#define fl_min(a,b) (a < b ? a : b)
#define fl_max(a,b) (a > b ? a : b)
void Fl_GDIplus_Graphics_Driver::loop(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3) {
  if (scale() != int(scale()) && ( (x0 == x3 && x1 == x2 && y0 == y1 && y3 == y2) ||
      (x0 == x1 && y1 == y2 && x2 == x3 && y3 == y0)) && line_width_ == 1) { // rectangular loop
    int left = fl_min(x0, fl_min(x1, fl_min(x2, x3)));
    int right = fl_max(x0, fl_max(x1, fl_max(x2, x3)));
    int top = fl_min(y0, fl_min(y1, fl_min(y2, y3)));
    int bottom = fl_max(y0, fl_max(y1, fl_max(y2, y3)));
    rect(left, top, right-left+1, bottom-top+1);
  } else {
    Gdiplus::GraphicsPath path;
    Gdiplus::PointF gdi2_p[4] = {Gdiplus::PointF(x0+1-line_width_/2.f, y0+1-line_width_/2.f), Gdiplus::PointF(x1+1-line_width_/2.f, y1+1-line_width_/2.f), Gdiplus::PointF(x2+1-line_width_/2.f, y2+1-line_width_/2.f), Gdiplus::PointF(x3+1-line_width_/2.f, y3+1-line_width_/2.f)};
    path.AddLines(gdi2_p, 4);
    path.CloseFigure();
    graphics_->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics_->DrawPath(pen_, &path);
    graphics_->SetSmoothingMode(Gdiplus::SmoothingModeDefault);
  }
}

void Fl_GDIplus_Graphics_Driver::polygon(int x0, int y0, int x1, int y1, int x2, int y2) {
  Gdiplus::GraphicsPath path;
  path.AddLine(x0, y0, x1, y1);
  path.AddLine(x1, y1, x2, y2);
  path.CloseFigure();
  graphics_->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
  graphics_->FillPath(brush_, &path);
  graphics_->SetSmoothingMode(Gdiplus::SmoothingModeDefault);
}

void Fl_GDIplus_Graphics_Driver::polygon(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3) {
  Gdiplus::GraphicsPath path;
  path.AddLine(x0, y0, x1, y1);
  path.AddLine(x1, y1, x2, y2);
  path.AddLine(x2, y2, x3, y3);
  path.CloseFigure();
  graphics_->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
  graphics_->FillPath(brush_, &path);
  graphics_->SetSmoothingMode(Gdiplus::SmoothingModeDefault);
}

// --- clipping

void Fl_GDIplus_Graphics_Driver::push_clip(int x, int y, int w, int h) {
  Fl_Region r;
  if (w > 0 && h > 0) {
    r = XRectangleRegion(x,y,w,h);
    Fl_Region current = rstack[rstackptr];
    if (current) {
      ((Gdiplus::Region*)r)->Intersect((Gdiplus::Region*)current);
    }
  } else { // make empty clip region:
    r = new Gdiplus::Region();
    ((Gdiplus::Region*)r)->MakeEmpty();
  }
  if (rstackptr < region_stack_max) rstack[++rstackptr] = r;
  else Fl::warning("Fl_GDI_Graphics_Driver::push_clip: clip stack overflow!\n");
  fl_restore_clip();
}

int Fl_GDIplus_Graphics_Driver::clip_box(int x, int y, int w, int h, int& X, int& Y, int& W, int& H){
  X = x; Y = y; W = w; H = h;
  Gdiplus::Region* r = (Gdiplus::Region*)rstack[rstackptr];
  if (!r) return 0;
  // The win32 API makes no distinction between partial and complete
  // intersection, so we have to check for partial intersection ourselves.
  Gdiplus::Region* rr = (Gdiplus::Region*)XRectangleRegion(x,y,w,h);
  Gdiplus::Region* temp = (Gdiplus::Region*)XRectangleRegion(x,y,w,h);
  int ret;
  temp->Intersect(r);
  if (temp->IsEmpty(graphics_)) { // disjoint
    W = H = 0;
    ret = 2;
  } else if (temp->Equals(rr, graphics_)) { // complete
    ret = 0;
  } else {      // partial intersection
    Gdiplus::Rect rect;
    temp->GetBounds(&rect, graphics_);
    X = rect.GetLeft(); Y = rect.GetTop();
    W = int(rect.GetRight()) - X + 1; H = int(rect.GetBottom()) - Y + 1;
    ret = 1;
  }
  delete temp;
  delete rr;
  return ret;
}

int Fl_GDIplus_Graphics_Driver::not_clipped(int x, int y, int w, int h) {
  if (x+w <= 0 || y+h <= 0) return 0;
  Fl_Region r = rstack[rstackptr];
  if (!r) return 1;
  Gdiplus::Region* r2 = (Gdiplus::Region*)XRectangleRegion(x, y, w, h);
  r2->Intersect((Gdiplus::Region*)r);
  int retval = !r2->IsEmpty(graphics_);
  delete r2;
  return retval;
}

void Fl_GDIplus_Graphics_Driver::restore_clip() {
  fl_clip_state_number++;
  if (graphics_) {
    Fl_Region r = rstack[rstackptr];
    if (r) {
       graphics_->SetClip((Gdiplus::Region*)r);
    } else {
      graphics_->ResetClip();
    }
  }
}

#else

// --- line and polygon drawing with integer coordinates

void Fl_GDI_Graphics_Driver::point(int x, int y) {
  rectf(x, y, 1, 1);
}

void Fl_GDI_Graphics_Driver::overlay_rect(int x, int y, int w , int h) {
  // make pen have a one-pixel width
  line_style_unscaled( (color()==FL_WHITE?FL_SOLID:FL_DOT), 1, NULL);
  int right = this->floor(x+w-1), bottom = this->floor(y+h-1);
  x = this->floor(x); y = this->floor(y);
  MoveToEx(gc_, x, y, 0L);
  LineTo(gc_, right, y);
  LineTo(gc_, right, bottom);
  LineTo(gc_, x, bottom);
  LineTo(gc_, x, y);
}

void Fl_GDI_Graphics_Driver::rect(int x, int y, int w, int h)
{
  if (w > 0 && h > 0) {
    xyline(x, y, (x+w-1));
    yxline(x, y, (y+h-1));
    yxline((x+w-1), y, (y+h-1));
    xyline(x, (y+h-1), (x+w-1));
  }
}

void Fl_GDI_Graphics_Driver::focus_rect(int x, int y, int w, int h) {
  // Windows 95/98/ME do not implement the dotted line style, so draw
  // every other pixel around the focus area...
  w = floor(x+w-1) - floor(x) + 1;
  h = floor(y+h-1) - floor(y) + 1;
  x = floor(x); y = floor(y);
  int i=1, xx, yy;
  COLORREF c = fl_RGB();
  for (xx = 0; xx < w; xx++, i++) if (i & 1) SetPixel(gc_, x+xx, y, c);
  for (yy = 0; yy < h; yy++, i++) if (i & 1) SetPixel(gc_, x+w, y+yy, c);
  for (xx = w; xx > 0; xx--, i++) if (i & 1) SetPixel(gc_, x+xx, y+h, c);
  for (yy = h; yy > 0; yy--, i++) if (i & 1) SetPixel(gc_, x, y+yy, c);
}

void Fl_GDI_Graphics_Driver::rectf(int x, int y, int w, int h) {
  if (w<=0 || h<=0) return;
  RECT rect;
  rect.left = this->floor(x); rect.top = this->floor(y);
  rect.right = this->floor(x + w); rect.bottom = this->floor(y + h);
  FillRect(gc_, &rect, fl_brush());
}

void Fl_GDI_Graphics_Driver::line_unscaled(float x, float y, float x1, float y1) {
  MoveToEx(gc_, int(x), int(y), 0L);
  LineTo(gc_, int(x1), int(y1));
  SetPixel(gc_, int(x1), int(y1), fl_RGB());
}

void Fl_GDI_Graphics_Driver::line_unscaled(float x, float y, float x1, float y1, float x2, float y2) {
  MoveToEx(gc_, int(x), int(y), 0L);
  LineTo(gc_, int(x1), int(y1));
  LineTo(gc_, int(x2), int(y2));
  SetPixel(gc_, int(x2), int(y2), fl_RGB());
}

static HPEN change_pen_width(int width, HDC gc) { // set the width of the pen, return previous pen
  LOGBRUSH penbrush = {BS_SOLID, fl_RGB(), 0};
  HPEN newpen = ExtCreatePen(PS_GEOMETRIC | PS_ENDCAP_FLAT | PS_JOIN_ROUND, width, &penbrush, 0, 0);
  return (HPEN)SelectObject(gc, newpen);
}

void Fl_GDI_Graphics_Driver::xyline(int x, int y, int x1) {
  if (y < 0) return;
  float s = scale();
  int xx = (x < x1 ? x : x1);
  int xx1 = (x < x1 ? x1 : x);
  if (s != int(s) && line_width_ <= int(s)) {
    int lwidth = this->floor((y+1)) - this->floor(y);
    bool need_pen = (lwidth != int(s));
    HPEN oldpen = (need_pen ? change_pen_width(lwidth, gc_) : NULL);
    MoveToEx(gc_, this->floor(xx), this->floor(y) + int(lwidth/2.f) , 0L);
    LineTo(gc_, this->floor((xx1+1)), this->floor(y) + int(lwidth/2.f));
    if (need_pen) {
      DeleteObject(SelectObject(gc_, oldpen));
    }
  } else {
    y = int((y + 0.5f) * s);
    MoveToEx(gc_, this->floor(xx), y, 0L);
    LineTo(gc_, this->floor(xx1) + int(s) , y);
  }
}

void Fl_GDI_Graphics_Driver::xyline(int x, int y, int x1, int y2) {
  xyline(x, y, x1);
  yxline(x1, y, y2);
}

void Fl_GDI_Graphics_Driver::xyline(int x, int y, int x1, int y2, int x3) {
  xyline(x, y, x1);
  yxline(x1, y, y2);
  xyline(x1, y2, x3);
}

void Fl_GDI_Graphics_Driver::yxline(int x, int y, int y1) {
  if (x < 0) return;
  double s = scale();
  int yy = (y < y1 ? y : y1);
  int yy1 = (y < y1 ? y1 : y);
  if (s != int(s) && line_width_ <= int(s)) {
    int lwidth = (this->floor((x+1)) - this->floor(x));
    bool need_pen = (lwidth != int(s));
    HPEN oldpen = (need_pen ? change_pen_width(lwidth, gc_) : NULL);
    MoveToEx(gc_, this->floor(x) + int(lwidth/2.f), this->floor(yy), 0L);
    LineTo(gc_, this->floor(x) + int(lwidth/2.f), this->floor((yy1+1)) );
    if (need_pen) {
      DeleteObject(SelectObject(gc_, oldpen));
    }
  } else {
    x = int((x + 0.5f) * s);
    MoveToEx(gc_, x, this->floor(yy), 0L);
    LineTo(gc_, x, this->floor(yy1) + int(s));
  }
}

void Fl_GDI_Graphics_Driver::yxline(int x, int y, int y1, int x2) {
  yxline(x, y, y1);
  xyline(x, y1, x2);
}

void Fl_GDI_Graphics_Driver::yxline(int x, int y, int y1, int x2, int y3) {
  yxline(x, y, y1);
  xyline(x, y1, x2);
  yxline(x2, y1, y3);
}

void Fl_GDI_Graphics_Driver::loop_unscaled(float x, float y, float x1, float y1, float x2, float y2) {
  MoveToEx(gc_, int(x), int(y), 0L);
  LineTo(gc_, int(x1), int(y1));
  LineTo(gc_, int(x2), int(y2));
  LineTo(gc_, int(x), int(y));
}

void Fl_GDI_Graphics_Driver::loop_unscaled(float x, float y, float x1, float y1, float x2, float y2, float x3, float y3) {
  MoveToEx(gc_, int(x), int(y), 0L);
  LineTo(gc_, int(x1), int(y1));
  LineTo(gc_, int(x2), int(y2));
  LineTo(gc_, int(x3), int(y3));
  LineTo(gc_, int(x), int(y));
}

void Fl_GDI_Graphics_Driver::polygon_unscaled(float x, float y, float x1, float y1, float x2, float y2) {
  POINT p[3];
  p[0].x = int(x);  p[0].y = int(y);
  p[1].x = int(x1); p[1].y = int(y1);
  p[2].x = int(x2); p[2].y = int(y2);
  SelectObject(gc_, fl_brush());
  Polygon(gc_, p, 3);
}

void Fl_GDI_Graphics_Driver::polygon_unscaled(float x, float y, float x1, float y1, float x2, float y2, float x3, float y3) {
  POINT p[4];
  p[0].x = int(x);  p[0].y = int(y);
  p[1].x = int(x1); p[1].y = int(y1);
  p[2].x = int(x2); p[2].y = int(y2);
  p[3].x = int(x3); p[3].y = int(y3);
  SelectObject(gc_, fl_brush());
  Polygon(gc_, p, 4);
}

// --- clipping

void Fl_GDI_Graphics_Driver::push_clip(int x, int y, int w, int h) {
  Fl_Region r;
  if (w > 0 && h > 0) {
    r = XRectangleRegion(x,y,w,h);
    Fl_Region current = rstack[rstackptr];
    if (current) {
      CombineRgn((HRGN)r,(HRGN)r,(HRGN)current,RGN_AND);
    }
  } else { // make empty clip region:
    r = CreateRectRgn(0,0,0,0);
  }
  if (rstackptr < region_stack_max) rstack[++rstackptr] = r;
  else Fl::warning("Fl_GDI_Graphics_Driver::push_clip: clip stack overflow!\n");
  fl_restore_clip();
}

int Fl_GDI_Graphics_Driver::clip_box(int x, int y, int w, int h, int& X, int& Y, int& W, int& H){
  X = x; Y = y; W = w; H = h;
  Fl_Region r = rstack[rstackptr];
  if (!r) return 0;
  // The win32 API makes no distinction between partial and complete
  // intersection, so we have to check for partial intersection ourselves.
  // However, given that the regions may be composite, we have to do
  // some voodoo stuff...
  HRGN rr = (HRGN)XRectangleRegion(x,y,w,h);
  HRGN temp = CreateRectRgn(0,0,0,0);
  int ret;
  if (CombineRgn(temp, rr, (HRGN)r, RGN_AND) == NULLREGION) { // disjoint
    W = H = 0;
    ret = 2;
  } else if (EqualRgn(temp, rr)) { // complete
    ret = 0;
  } else {      // partial intersection
    RECT rect;
    GetRgnBox(temp, &rect);
    if (Fl_Surface_Device::surface() != Fl_Display_Device::display_device()) { // if print context, convert coords from device to logical
      POINT pt[2] = { {rect.left, rect.top}, {rect.right, rect.bottom} };
      DPtoLP(gc_, pt, 2);
      X = pt[0].x; Y = pt[0].y; W = pt[1].x - X; H = pt[1].y - Y;
    }
    else {
      X = rect.left; Y = rect.top; W = rect.right - X; H = rect.bottom - Y;
    }
    ret = 1;
  }
  DeleteObject(temp);
  DeleteObject(rr);
  return ret;
}

int Fl_GDI_Graphics_Driver::not_clipped(int x, int y, int w, int h) {
  if (x+w <= 0 || y+h <= 0) return 0;
  Fl_Region r = rstack[rstackptr];
  if (!r) return 1;
  RECT rect;
  if (Fl_Surface_Device::surface() != Fl_Display_Device::display_device()) { // in case of print context, convert coords from logical to device
    POINT pt[2] = { {x, y}, {x + w, y + h} };
    LPtoDP(gc_, pt, 2);
    rect.left = pt[0].x; rect.top = pt[0].y; rect.right = pt[1].x; rect.bottom = pt[1].y;
  } else {
    rect.left = x; rect.top = y; rect.right = x+w; rect.bottom = y+h;
  }
  return RectInRegion((HRGN)r,&rect);
}

void Fl_GDI_Graphics_Driver::restore_clip() {
  fl_clip_state_number++;
  if (gc_) {
    HRGN r = NULL;
    if (rstack[rstackptr]) r = (HRGN)scale_clip(scale());
    SelectClipRgn(gc_, (HRGN)rstack[rstackptr]); // if region is NULL, clip is automatically cleared
    if (r) unscale_clip(r);
  }
}

#endif
