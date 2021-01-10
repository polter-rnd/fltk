//
// Copy-to-clipboard code for the Fast Light Tool Kit (FLTK).
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
#include <FL/Fl_Copy_Surface.H>
#include <FL/platform.H>
#include "Fl_GDI_Graphics_Driver.H"
#include "../WinAPI/Fl_WinAPI_Screen_Driver.H"
#include <FL/Fl_Image_Surface.H>
#include <windows.h>

class Fl_GDI_Copy_Surface_Driver : public Fl_Copy_Surface_Driver {
  friend class Fl_Copy_Surface_Driver;
protected:
  HDC oldgc;
  HDC gc;
  Fl_GDI_Copy_Surface_Driver(int w, int h);
  ~Fl_GDI_Copy_Surface_Driver();
  void set_current();
  void translate(int x, int y);
  void untranslate();
};


Fl_Copy_Surface_Driver *Fl_Copy_Surface_Driver::newCopySurfaceDriver(int w, int h)
{
  return new Fl_GDI_Copy_Surface_Driver(w, h);
}


Fl_GDI_Copy_Surface_Driver::Fl_GDI_Copy_Surface_Driver(int w, int h) : Fl_Copy_Surface_Driver(w, h) {
#if USE_GDIPLUS
  driver(new Fl_GDIplus_Graphics_Driver);
#else
  driver(new Fl_GDI_Graphics_Driver);
#endif
  oldgc = (HDC)Fl_Surface_Device::surface()->driver()->gc();
  // exact computation of factor from screen units to EnhMetaFile units (0.01 mm)
  HDC hdc = GetDC(NULL);
  int hmm = GetDeviceCaps(hdc, HORZSIZE);
  int hdots = GetDeviceCaps(hdc, HORZRES);
  int vmm = GetDeviceCaps(hdc, VERTSIZE);
  int vdots = GetDeviceCaps(hdc, VERTRES);
  ReleaseDC(NULL, hdc);
  float factorw = (100.f * hmm) / hdots;
  float factorh = (100.f * vmm) / vdots;
  // Global display scaling factor: 1, 1.25, 1.5, 1.75, etc...
  float scaling = Fl_Graphics_Driver::default_driver().scale();
  driver()->scale(scaling);
  RECT rect; rect.left = 0; rect.top = 0; rect.right = (LONG)((w*scaling) * factorw); rect.bottom = (LONG)((h*scaling) * factorh);
  gc = CreateEnhMetaFile (NULL, NULL, &rect, NULL);
  if (gc != NULL) {
    SetTextAlign(gc, TA_BASELINE|TA_LEFT);
    SetBkMode(gc, TRANSPARENT);
#if USE_GDIPLUS
    ((Fl_GDIplus_Graphics_Driver*)driver())->graphics_ = new Gdiplus::Graphics(gc);
    ((Fl_GDIplus_Graphics_Driver*)driver())->graphics_->ScaleTransform(scaling, scaling);
#endif
  }
}


Fl_GDI_Copy_Surface_Driver::~Fl_GDI_Copy_Surface_Driver() {
  if (oldgc == (HDC)Fl_Surface_Device::surface()->driver()->gc()) oldgc = NULL;
  HENHMETAFILE hmf = CloseEnhMetaFile (gc);
  if ( hmf != NULL ) {
    if ( OpenClipboard (NULL) ){
      EmptyClipboard ();
      // put first the vectorial form of the graphics in the clipboard
      SetClipboardData (CF_ENHMETAFILE, hmf);
      // then put a BITMAP version of the graphics in the clipboard
<<<<<<< HEAD
      float scaling = driver()->scale();
<<<<<<< HEAD
      int W = Fl_GDI_Graphics_Driver::floor(width, scaling), H = Fl_GDI_Graphics_Driver::floor(height, scaling);
=======
      int W = int(width * scaling), H = int(height * scaling);
=======
      float scaling = Fl_Graphics_Driver::default_driver().scale();
      int W = width * scaling, H = height * scaling;
>>>>>>> Add option to have Windows platform use GDI+ rather that GDI
>>>>>>> Add option to have Windows platform use GDI+ rather that GDI
      RECT rect = {0, 0, W, H};
      Fl_Image_Surface *surf = new Fl_Image_Surface(W, H);
      Fl_Surface_Device::push_current(surf);
      fl_color(FL_WHITE);    // draw white background
      fl_rectf(0, 0, W, H);
#if USE_GDIPLUS
      HDC hdc = ((Fl_GDIplus_Graphics_Driver*)surf->driver())->graphics_->GetHDC();
#else
      HDC hdc = (HDC)surf->driver()->gc();
#endif
      PlayEnhMetaFile(hdc, hmf, &rect); // draw metafile to offscreen buffer
#if USE_GDIPLUS
      ((Fl_GDIplus_Graphics_Driver*)surf->driver())->graphics_->ReleaseHDC(hdc);
      Gdiplus::Bitmap *gdi_bm = (Gdiplus::Bitmap*)surf->offscreen();
      HBITMAP hbm;
      Gdiplus::Status st = gdi_bm->GetHBITMAP(Gdiplus::Color(255, 255, 255), &hbm);
      if (st == Gdiplus::Ok) {
        SetClipboardData(CF_BITMAP, hbm);
        DeleteObject(hbm);
      }
#else
      SetClipboardData(CF_BITMAP, surf->offscreen());
#endif
      Fl_Surface_Device::pop_current();
      delete surf;

      CloseClipboard ();
    }
    DeleteEnhMetaFile(hmf);
  }
  DeleteDC(gc);
  Fl_Surface_Device::surface()->driver()->gc(oldgc);
  delete driver();
}


void Fl_GDI_Copy_Surface_Driver::set_current() {
  driver()->gc(gc);
  fl_window = (Window)-1;
  Fl_Surface_Device::set_current();
}


void Fl_GDI_Copy_Surface_Driver::translate(int x, int y) {
#if USE_GDIPLUS
  ((Fl_GDIplus_Graphics_Driver*)driver())->translate_all(x, y);
#else
  ((Fl_GDI_Graphics_Driver*)driver())->translate_all(x, y);
#endif
}


void Fl_GDI_Copy_Surface_Driver::untranslate() {
#if USE_GDIPLUS
  ((Fl_GDIplus_Graphics_Driver*)driver())->untranslate_all();
#else
  ((Fl_GDI_Graphics_Driver*)driver())->untranslate_all();
#endif
}
