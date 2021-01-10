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


#include <config.h>
#include "Fl_GDI_Graphics_Driver.H"
#include <FL/Fl.H>
#include <FL/platform.H>
#include <FL/fl_draw.H>
#include "../../Fl_Screen_Driver.H"

/* Reference to the current device context
 For back-compatibility only. The preferred procedure to get this reference is
 Fl_Surface_Device::surface()->driver()->gc().
 */
HDC fl_gc = 0;


typedef BOOL(WINAPI* flTypeImmAssociateContextEx)(HWND, HIMC, DWORD);
extern flTypeImmAssociateContextEx flImmAssociateContextEx;
typedef HIMC(WINAPI* flTypeImmGetContext)(HWND);
extern flTypeImmGetContext flImmGetContext;
typedef BOOL(WINAPI* flTypeImmSetCompositionWindow)(HIMC, LPCOMPOSITIONFORM);
extern flTypeImmSetCompositionWindow flImmSetCompositionWindow;
typedef BOOL(WINAPI* flTypeImmReleaseContext)(HWND, HIMC);
extern flTypeImmReleaseContext flImmReleaseContext;


void
#if USE_GDIPLUS
  Fl_GDIplus_Graphics_Driver
#else
  Fl_GDI_Graphics_Driver
#endif
    ::set_spot(int font, int size, int X, int Y, int W, int H, Fl_Window *win)
{
  if (!win) return;
  Fl_Window* tw = win;
  while (tw->parent()) tw = tw->window(); // find top level window

  if (!tw->shown())
    return;

  HIMC himc = flImmGetContext(fl_xid(tw));

  if (himc) {
    COMPOSITIONFORM cfs;
    cfs.dwStyle = CFS_POINT;
    cfs.ptCurrentPos.x = X;
    cfs.ptCurrentPos.y = Y - tw->labelsize();
    MapWindowPoints(fl_xid(win), fl_xid(tw), &cfs.ptCurrentPos, 1);
    flImmSetCompositionWindow(himc, &cfs);
    flImmReleaseContext(fl_xid(tw), himc);
  }
}


#if USE_GDIPLUS

Fl_GDIplus_Graphics_Driver::Fl_GDIplus_Graphics_Driver() {
  mask_bitmap_ = NULL;
  gc_ = NULL;
  p_size = 0;
  p = NULL;
  graphics_ = NULL;
  translate_stack_depth = 0;
  brush_ = new Gdiplus::SolidBrush(Gdiplus::Color());
  pen_ = new Gdiplus::Pen(Gdiplus::Color(), 1);
}

Fl_GDIplus_Graphics_Driver::~Fl_GDIplus_Graphics_Driver() {
  if (p) free(p);
  delete graphics_;
  delete brush_;
  delete pen_;
}

static ULONG_PTR gdiplusToken = 0;
Gdiplus::StringFormat *Fl_GDIplus_Graphics_Driver::format = NULL;

/*
 * By linking this module, the following static method will instantiate the
 * Windows GDI Graphics driver as the main display driver.
 */
Fl_Graphics_Driver *Fl_Graphics_Driver::newMainGraphicsDriver()
{
  // Initialize GDI+.
  static Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  if (gdiplusToken == 0) GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
  
  Fl_Graphics_Driver *driver = new Fl_GDIplus_Graphics_Driver();
  Fl_GDIplus_Graphics_Driver::format = (Gdiplus::StringFormat *)Gdiplus::StringFormat::GenericTypographic();
  Fl_GDIplus_Graphics_Driver::format->SetFormatFlags(Gdiplus::StringFormatFlagsMeasureTrailingSpaces);
  return driver;
}

void Fl_GDIplus_Graphics_Driver::shutdown() {
  Gdiplus::GdiplusShutdown(gdiplusToken);
}

void Fl_GDIplus_Graphics_Driver::gc(void *ctxt) {
  gc_ = (HDC)ctxt;
  global_gc();
}

void Fl_GDIplus_Graphics_Driver::global_gc()
{
  fl_gc = (HDC)gc();
}

/*
 * This function checks if the version of Windows that we
 * curently run on supports alpha blending for bitmap transfers
 * and finds the required function if so.
 */
char Fl_GDIplus_Graphics_Driver::can_do_alpha_blending() {
  return 1;
}

void Fl_GDIplus_Graphics_Driver::copy_offscreen(int x, int y, int w, int h, Fl_Offscreen bitmap, int srcx, int srcy) {
  if (srcx < 0) {w += srcx; x -= srcx; srcx = 0;}
  if (srcy < 0) {h += srcy; y -= srcy; srcy = 0;}
  int off_width = ((Gdiplus::Bitmap*)bitmap)->GetWidth()/scale();
  int off_height = ((Gdiplus::Bitmap*)bitmap)->GetHeight()/scale();
  if (srcx + w >= off_width) {w = off_width - srcx;}
  if (srcy + h >= off_height) {h = off_height - srcy;}
  if (w <= 0 || h <= 0) return;
  push_clip(x, y, w, h);
  graphics_->DrawImage(((Gdiplus::Bitmap*)bitmap), Gdiplus::Rect(x-srcx, y-srcy, off_width, off_height));
  pop_clip();
}

void Fl_GDIplus_Graphics_Driver::add_rectangle_to_region(Fl_Region r, int X, int Y, int W, int H) {
  ((Gdiplus::Region*)r)->Union(Gdiplus::Rect(X,Y,W,H));
}

void Fl_GDIplus_Graphics_Driver::transformed_vertex0(float x, float y) {
  if (!n || x != p[n-1].x || y != p[n-1].y) {
    if (n >= p_size) {
      p_size = p ? 2*p_size : 16;
      p = (POINT*)realloc((void*)p, p_size*sizeof(*p));
    }
    p[n].x = x;
    p[n].y = y;
    n++;
  }
}

void Fl_GDIplus_Graphics_Driver::fixloop() {  // remove equal points from closed path
  while (n>2 && p[n-1].x == p[0].x && p[n-1].y == p[0].y) n--;
}

Fl_Region Fl_GDIplus_Graphics_Driver::XRectangleRegion(int x, int y, int w, int h) {
  return new Gdiplus::Region(Gdiplus::Rect(x,y,w,h));
}

void Fl_GDIplus_Graphics_Driver::XDestroyRegion(Fl_Region r) {
  delete (Gdiplus::Region*)r;
}

void Fl_GDIplus_Graphics_Driver::scale(float f) {
  if (f != scale()) {
    size_ = 0;
    Fl_Graphics_Driver::scale(f);
    line_style(FL_SOLID); // scale also default line width
  }
}

const int Fl_GDIplus_Graphics_Driver::translate_stack_max = 5;

void Fl_GDIplus_Graphics_Driver::translate_all(int x, int y)
{
  if (translate_stack_depth < translate_stack_max) {
    translate_stack[translate_stack_depth++] = graphics_->BeginContainer();
    graphics_->TranslateTransform(x, y);
    }
}

void Fl_GDIplus_Graphics_Driver::untranslate_all(void)
{
  if (translate_stack_depth > 0) {
    graphics_->EndContainer(translate_stack[--translate_stack_depth]);
    }
}

void Fl_GDIplus_Graphics_Driver::set_current_() {
  restore_clip();
}

void Fl_GDIplus_Graphics_Driver::arc(int x, int y, int w, int h, double a1, double a2) {
  if (w <= 0 || h <= 0) return;
  graphics_->DrawArc(pen_, x, y, w, h, -a1, -(a2-a1));
}

void Fl_GDIplus_Graphics_Driver::pie(int x, int y, int w, int h, double a1, double a2) {
  if (w <= 0 || h <= 0) return;
  graphics_->FillPie(brush_, x, y, w, h, -a1, -(a2-a1));
}

Fl_RGB_Image *Fl_GDIplus_Graphics_Driver::offscreen_to_rgb(Fl_Offscreen offscreen) {
  int w = ((Gdiplus::Bitmap*)offscreen)->GetWidth(), h = ((Gdiplus::Bitmap*)offscreen)->GetHeight();
  Gdiplus::Rect rect(0, 0, w, h);
  Gdiplus::BitmapData bmdata;
  int ld = ((3*w+3)/4)*4;
  uchar *array = new uchar[ld*h];
  bmdata.Width = w;
  bmdata.Height = h;
  bmdata.Stride = ld;
  bmdata.PixelFormat = PixelFormat24bppRGB;
  bmdata.Scan0 = array;
  ((Gdiplus::Bitmap*)offscreen)->LockBits(&rect, Gdiplus::ImageLockModeUserInputBuf | Gdiplus::ImageLockModeRead, PixelFormat24bppRGB, &bmdata);
  ((Gdiplus::Bitmap*)offscreen)->UnlockBits(&bmdata);
  uchar *from = array;
  for (int i = 0; i < h; i++) { // convert BGR to RGB
    for (uchar* p = from; p < from+3*w; p += 3) {
      uchar q = *p; // exchange R and B pixels
      *p = *(p+2);
      *(p+2) = q;
    }
    from += ld;
  }
  Fl_RGB_Image *image = new Fl_RGB_Image(array, w, h, 3, ld);
  image->alloc_array = 1;
  return image;
}

void Fl_GDIplus_Graphics_Driver::cache_size(Fl_Image *img, int &width, int &height) {
  width *= 2 * scale();
  height *= 2 * scale();
}

#else

/*
 * By linking this module, the following static method will instantiate the
 * Windows GDI Graphics driver as the main display driver.
 */
Fl_Graphics_Driver *Fl_Graphics_Driver::newMainGraphicsDriver()
{
  return new Fl_GDI_Graphics_Driver();
}

// Code used to switch output to an off-screen window.  See macros in
// win32.H which save the old state in local variables.

typedef struct { BYTE a; BYTE b; BYTE c; BYTE d; } FL_BLENDFUNCTION;
typedef BOOL (WINAPI* fl_alpha_blend_func)
(HDC,int,int,int,int,HDC,int,int,int,int,FL_BLENDFUNCTION);
static fl_alpha_blend_func fl_alpha_blend = NULL;
static FL_BLENDFUNCTION blendfunc = { 0, 0, 255, 1};

void Fl_GDI_Graphics_Driver::global_gc()
{
  fl_gc = (HDC)gc();
}

/*
 * This function checks if the version of Windows that we
 * curently run on supports alpha blending for bitmap transfers
 * and finds the required function if so.
 */
char Fl_GDI_Graphics_Driver::can_do_alpha_blending() {
  static char been_here = 0;
  static char can_do = 0;
  // do this test only once
  if (been_here) return can_do;
  been_here = 1;
  // load the library that implements alpha blending
  HMODULE hMod = LoadLibrary("MSIMG32.DLL");
  // give up if that doesn't exist (Win95?)
  if (!hMod) return 0;
  // now find the blending function inside that dll
  fl_alpha_blend = (fl_alpha_blend_func)GetProcAddress(hMod, "AlphaBlend");
  // give up if we can't find it (Win95)
  if (!fl_alpha_blend) return 0;
  // we have the call, but does our display support alpha blending?
  // get the desktop's device context
  HDC dc = GetDC(0L);
  if (!dc) return 0;
  // check the device capabilities flags. However GetDeviceCaps
  // does not return anything useful, so we have to do it manually:

  HBITMAP bm = CreateCompatibleBitmap(dc, 1, 1);
  HDC new_gc = CreateCompatibleDC(dc);
  int save = SaveDC(new_gc);
  SelectObject(new_gc, bm);
  /*COLORREF set = */ SetPixel(new_gc, 0, 0, 0x01010101);
  BOOL alpha_ok = fl_alpha_blend(dc, 0, 0, 1, 1, new_gc, 0, 0, 1, 1, blendfunc);
  RestoreDC(new_gc, save);
  DeleteDC(new_gc);
  DeleteObject(bm);
  ReleaseDC(0L, dc);

  if (alpha_ok) can_do = 1;
  return can_do;
}

HDC fl_makeDC(HBITMAP bitmap) {
  HDC new_gc = CreateCompatibleDC((HDC)Fl_Graphics_Driver::default_driver().gc());
  SetTextAlign(new_gc, TA_BASELINE|TA_LEFT);
  SetBkMode(new_gc, TRANSPARENT);
#if USE_COLORMAP
  if (fl_palette) SelectPalette(new_gc, fl_palette, FALSE);
#endif
  SelectObject(new_gc, bitmap);
  return new_gc;
}

void Fl_GDI_Graphics_Driver::copy_offscreen(int x, int y, int w, int h, Fl_Offscreen bitmap, int srcx, int srcy) {
  x = int(x * scale()); y = int(y * scale()); w = int(w * scale()); h = int(h * scale());
  srcx = int(srcx * scale()); srcy = int(srcy * scale());
  if (srcx < 0) {w += srcx; x -= srcx; srcx = 0;}
  if (srcy < 0) {h += srcy; y -= srcy; srcy = 0;}
  int off_width, off_height;
  Fl::screen_driver()->offscreen_size(bitmap, off_width, off_height);
  if (srcx + w >= off_width) {w = off_width - srcx;}
  if (srcy + h >= off_height) {h = off_height - srcy;}
  if (w <= 0 || h <= 0) return;
  HDC new_gc = CreateCompatibleDC(gc_);
  int save = SaveDC(new_gc);
  SelectObject(new_gc, (HBITMAP)bitmap);
  BitBlt(gc_, x, y, w, h, new_gc, srcx, srcy, SRCCOPY);
  RestoreDC(new_gc, save);
  DeleteDC(new_gc);
}

void Fl_GDI_Printer_Graphics_Driver::copy_offscreen(int x, int y, int w, int h, Fl_Offscreen bitmap, int srcx, int srcy) {
  Fl_Graphics_Driver::copy_offscreen(x, y, w, h, bitmap, srcx, srcy);
}

BOOL Fl_GDI_Graphics_Driver::alpha_blend_(int x, int y, int w, int h, HDC src_gc, int srcx, int srcy, int srcw, int srch) {
  return fl_alpha_blend(gc_, x, y, w, h, src_gc, srcx, srcy, srcw, srch, blendfunc);
}

#if ! defined(FL_DOXYGEN)
void Fl_GDI_Graphics_Driver::copy_offscreen_with_alpha(int x,int y,int w,int h,HBITMAP bitmap,int srcx,int srcy) {
  HDC new_gc = CreateCompatibleDC(gc_);
  int save = SaveDC(new_gc);
  SelectObject(new_gc, bitmap);
  BOOL alpha_ok = 0;
  // first try to alpha blend
  if ( fl_can_do_alpha_blending() ) {
    alpha_ok = alpha_blend_(x, y, w, h, new_gc, srcx, srcy, w, h);
  }
  // if that failed (it shouldn't), still copy the bitmap over, but now alpha is 1
  if (!alpha_ok) {
    BitBlt(gc_, x, y, w, h, new_gc, srcx, srcy, SRCCOPY);
  }
  RestoreDC(new_gc, save);
  DeleteDC(new_gc);
}

void Fl_GDI_Graphics_Driver::translate_all(int x, int y) {
  const int stack_height = 10;
  if (depth == -1) {
    origins = new POINT[stack_height];
    depth = 0;
  }
  if (depth >= stack_height)  {
    Fl::warning("Fl_Copy/Image_Surface: translate stack overflow!");
    depth = stack_height - 1;
  }
  GetWindowOrgEx((HDC)gc(), origins+depth);
  SetWindowOrgEx((HDC)gc(), int(origins[depth].x - x*scale()), int(origins[depth].y - y*scale()), NULL);
  depth++;
}

void Fl_GDI_Graphics_Driver::untranslate_all() {
  if (depth > 0) depth--;
  SetWindowOrgEx((HDC)gc(), origins[depth].x, origins[depth].y, NULL);
}
#endif

void Fl_GDI_Graphics_Driver::add_rectangle_to_region(Fl_Region r, int X, int Y, int W, int H) {
  Fl_Region R = XRectangleRegion(X, Y, W, H);
  CombineRgn((HRGN)r, (HRGN)r, (HRGN)R, RGN_OR);
  XDestroyRegion(R);
}

void Fl_GDI_Graphics_Driver::transformed_vertex0(float x, float y) {
  if (!n || x != p[n-1].x || y != p[n-1].y) {
    if (n >= p_size) {
      p_size = p ? 2*p_size : 16;
      p = (POINT*)realloc((void*)p, p_size*sizeof(*p));
    }
    p[n].x = int(x);
    p[n].y = int(y);
    n++;
  }
}

void Fl_GDI_Graphics_Driver::fixloop() {  // remove equal points from closed path
  while (n>2 && p[n-1].x == p[0].x && p[n-1].y == p[0].y) n--;
}

Fl_Region Fl_GDI_Graphics_Driver::XRectangleRegion(int x, int y, int w, int h) {
  if (Fl_Surface_Device::surface() == Fl_Display_Device::display_device()) return CreateRectRgn(x,y,x+w,y+h);
  // because rotation may apply, the rectangle becomes a polygon in device coords
  POINT pt[4] = { {x, y}, {x + w, y}, {x + w, y + h}, {x, y + h} };
  LPtoDP((HDC)fl_graphics_driver->gc(), pt, 4);
  return CreatePolygonRgn(pt, 4, ALTERNATE);
}

void Fl_GDI_Graphics_Driver::XDestroyRegion(Fl_Region r) {
  DeleteObject((HRGN)r);
}


void Fl_GDI_Graphics_Driver::scale(float f) {
  if (f != scale()) {
    size_ = 0;
    Fl_Graphics_Driver::scale(f);
    line_style(FL_SOLID); // scale also default line width
  }
}


/* Rescale region r with factor f and returns the scaled region.
 Region r is returned unchanged if r is null or f is 1.
 */
HRGN Fl_GDI_Graphics_Driver::scale_region(HRGN r, float f, Fl_GDI_Graphics_Driver *dr) {
  if (r && f != 1) {
    DWORD size = GetRegionData(r, 0, NULL);
    RGNDATA *pdata = (RGNDATA*)malloc(size);
    GetRegionData(r, size, pdata);
    POINT pt = {0, 0};
    if (dr && dr->depth >= 1) { // account for translation
      GetWindowOrgEx((HDC)dr->gc(), &pt);
      pt.x = int(pt.x * (f - 1));
      pt.y = int(pt.y * (f - 1));
    }
    RECT *rects = (RECT*)&(pdata->Buffer);
    for (DWORD i = 0; i < pdata->rdh.nCount; i++) {
      int x = Fl_GDI_Graphics_Driver::floor(rects[i].left, f) + pt.x;
      int y = Fl_GDI_Graphics_Driver::floor(rects[i].top, f) + pt.y;
      RECT R2;
      R2.left = x;
      R2.top  = y;
      R2.right = Fl_GDI_Graphics_Driver::floor(rects[i].right, f) + pt.x - x + R2.left;
      R2.bottom = Fl_GDI_Graphics_Driver::floor(rects[i].bottom, f) + pt.y - y + R2.top;
      rects[i] = R2;
    }
    r = ExtCreateRegion(NULL, size, pdata);
    free(pdata);
  }
  return r;
}


Fl_Region Fl_GDI_Graphics_Driver::scale_clip(float f) {
  HRGN r = (HRGN)rstack[rstackptr];
  HRGN r2 = (HRGN)scale_region(r, f, this);
  return (r == r2 ? NULL : (rstack[rstackptr] = r2, r));
}

void Fl_GDI_Graphics_Driver::set_current_() {
  restore_clip();
}

<<<<<<< HEAD
void Fl_GDI_Graphics_Driver::cache_size(Fl_Image *img, int &width, int &height)
{
  float s = scale();
  width  = (s == int(s) ? width * int(s) : floor(width+1));
  height = (s == int(s) ? height * int(s) : floor(height+1));
  cache_size_finalize(img, width, height);
}
=======
#endif
>>>>>>> Add option to have Windows platform use GDI+ rather that GDI
