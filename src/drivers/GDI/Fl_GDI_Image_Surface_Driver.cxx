//
// Draw-to-image code for the Fast Light Tool Kit (FLTK).
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

#include <config.h>
#include "Fl_GDI_Graphics_Driver.H"
#include "../WinAPI/Fl_WinAPI_Screen_Driver.H"
#include <FL/Fl_Image_Surface.H>
#include <FL/platform.H>
#include <windows.h>

class Fl_GDI_Image_Surface_Driver : public Fl_Image_Surface_Driver {
  virtual void end_current();
public:
  Window pre_window;
#if !USE_GDIPLUS
  int _savedc;
  POINT origin;
#endif
  Fl_GDI_Image_Surface_Driver(int w, int h, int high_res, Fl_Offscreen off);
  ~Fl_GDI_Image_Surface_Driver();
  void set_current();
  void translate(int x, int y);
  void untranslate();
  Fl_RGB_Image *image();
};


Fl_Image_Surface_Driver *Fl_Image_Surface_Driver::newImageSurfaceDriver(int w, int h, int high_res, Fl_Offscreen off)
{
  return new Fl_GDI_Image_Surface_Driver(w, h, high_res, off);
}


Fl_GDI_Image_Surface_Driver::Fl_GDI_Image_Surface_Driver(int w, int h, int high_res, Fl_Offscreen off) : Fl_Image_Surface_Driver(w, h, high_res, off) {
  float d =  fl_graphics_driver->scale();
  if (!off && d != 1 && high_res) {
#if USE_GDIPLUS
    w = int((w+1)*d)-1;
    h = int((h+1)*d)-1;
#else
    w = int(w*d);
    h = int(h*d);
#endif
  }
#if USE_GDIPLUS
  if (!off) {
    offscreen = new Gdiplus::Bitmap(w, h, PixelFormat32bppARGB);
  } else { offscreen = off; }
  driver(new Fl_GDIplus_Graphics_Driver);
  if (off || !high_res) d = 1;
  ((Fl_GDIplus_Graphics_Driver*)driver())->graphics( new Gdiplus::Graphics((Gdiplus::Bitmap*)offscreen) );
  ((Fl_GDIplus_Graphics_Driver*)driver())->graphics()->ScaleTransform(d, d);
  driver()->scale(d);
#else
  HDC gc = (HDC)Fl_Graphics_Driver::default_driver().gc();
  offscreen = off ? off : CreateCompatibleBitmap( (gc ? gc : fl_GetDC(0) ) , w, h);
  if (!offscreen) offscreen = CreateCompatibleBitmap(fl_GetDC(0), w, h);
  driver(new Fl_GDI_Graphics_Driver);
  if (d != 1 && high_res) ((Fl_GDI_Graphics_Driver*)driver())->scale(d);
  origin.x = origin.y = 0;
#endif
}


Fl_GDI_Image_Surface_Driver::~Fl_GDI_Image_Surface_Driver() {
#if USE_GDIPLUS
  if (!external_offscreen) delete (Gdiplus::Bitmap*)offscreen;
#else
  if (offscreen && !external_offscreen) DeleteObject(offscreen);
#endif
  delete driver();
}


void Fl_GDI_Image_Surface_Driver::set_current() {
#if !USE_GDIPLUS
  HDC gc = fl_makeDC((HBITMAP)offscreen);
  driver()->gc(gc);
  SetWindowOrgEx(gc, origin.x, origin.y, NULL);
#endif
  Fl_Surface_Device::set_current();
  pre_window = fl_window;
#if !USE_GDIPLUS
  _savedc = SaveDC(gc);
#endif
  fl_window=(HWND)offscreen;
}


void Fl_GDI_Image_Surface_Driver::translate(int x, int y) {
#if USE_GDIPLUS
  ((Fl_GDIplus_Graphics_Driver*)driver())->translate_all(x, y);
#else
  ((Fl_GDI_Graphics_Driver*)driver())->translate_all(x, y);
#endif
}


void Fl_GDI_Image_Surface_Driver::untranslate() {
#if USE_GDIPLUS
  ((Fl_GDIplus_Graphics_Driver*)driver())->untranslate_all();
#else
  ((Fl_GDI_Graphics_Driver*)driver())->untranslate_all();
#endif
}


Fl_RGB_Image* Fl_GDI_Image_Surface_Driver::image()
{
  Fl_RGB_Image *image =
#if USE_GDIPLUS
    Fl_GDIplus_Graphics_Driver::offscreen_to_rgb(offscreen);
  image->scale(width, height, 0, 1);
#else
    Fl::screen_driver()->read_win_rectangle( 0, 0, width, height, 0);
#endif
  return image;
}


void Fl_GDI_Image_Surface_Driver::end_current()
{
#if !USE_GDIPLUS
  HDC gc = (HDC)driver()->gc();
  RestoreDC(gc, _savedc);
  DeleteDC(gc);
#endif
  fl_window = pre_window;
  Fl_Surface_Device::end_current();
}
