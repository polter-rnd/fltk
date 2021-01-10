//
// Printing support for Windows for the Fast Light Tool Kit (FLTK).
//
// Copyright 2010-2020 by Bill Spitzak and others.
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

#include "../GDI/Fl_GDI_Graphics_Driver.H"
#include <FL/Fl_Paged_Device.H>
#include <FL/Fl_Printer.H>
#include <FL/fl_ask.H>
#include <FL/math.h>
#include <FL/fl_draw.H>
#include <commdlg.h>

extern HWND fl_window;

/** Support for printing on the Windows platform */
class Fl_WinAPI_Printer_Driver : public Fl_Paged_Device {
  friend class Fl_Printer;
private:
  int   abortPrint;
  PRINTDLG      pd;
  HDC           hPr;
  int           prerr;
  int left_margin;
  int top_margin;
#if USE_GDIPLUS
  int printable_width, printable_height;
  Gdiplus::GraphicsState initial_state;
  float scale_x_, scale_y_;
  float
#else
  void
#endif
      absolute_printable_rect(int *x, int *y, int *w, int *h);
  Fl_WinAPI_Printer_Driver(void);
  int begin_job(int pagecount = 0, int *frompage = NULL, int *topage = NULL, char **perr_message = NULL);
  int begin_page (void);
  int printable_rect(int *w, int *h);
  void margins(int *left, int *top, int *right, int *bottom);
  void origin(int *x, int *y);
  void origin(int x, int y);
  void scale (float scale_x, float scale_y = 0.);
  void rotate(float angle);
  void translate(int x, int y);
  void untranslate(void);
  int end_page (void);
  void end_job (void);
  ~Fl_WinAPI_Printer_Driver(void);
};

Fl_WinAPI_Printer_Driver::Fl_WinAPI_Printer_Driver(void) : Fl_Paged_Device() {
  hPr = NULL;
#if USE_GDIPLUS
  scale_x_ = scale_y_ = 1;
  driver(new Fl_GDIplus_Graphics_Driver);
#else
  driver(new Fl_GDI_Printer_Graphics_Driver);
#endif
}

Fl_Paged_Device* Fl_Printer::newPrinterDriver(void)
{
  return new Fl_WinAPI_Printer_Driver();
}


Fl_WinAPI_Printer_Driver::~Fl_WinAPI_Printer_Driver(void) {
  if (hPr) end_job();
  delete driver();
}

static void WIN_SetupPrinterDeviceContext(HDC prHDC)
{
  if ( !prHDC ) return;

  fl_window = 0;
  SetGraphicsMode(prHDC, GM_ADVANCED); // to allow for rotations
  SetMapMode(prHDC, MM_ANISOTROPIC);
  SetTextAlign(prHDC, TA_BASELINE|TA_LEFT);
  SetBkMode(prHDC, TRANSPARENT);
  // this matches 720 logical units to the number of device units in 10 inches of paper
  // thus the logical unit is the point (= 1/72 inch)
  SetWindowExtEx(prHDC, 720, 720, NULL);
  SetViewportExtEx(prHDC, 10*GetDeviceCaps(prHDC, LOGPIXELSX), 10*GetDeviceCaps(prHDC, LOGPIXELSY), NULL);
}


int Fl_WinAPI_Printer_Driver::begin_job (int pagecount, int *frompage, int *topage, char **perr_message)
// returns 0 iff OK
{
  if (pagecount == 0) pagecount = 10000;
  DOCINFO     di;
  char        docName [256];
  int err = 0;

  abortPrint = FALSE;
  memset (&pd, 0, sizeof (PRINTDLG));
  pd.lStructSize = sizeof (PRINTDLG);
  pd.hwndOwner = GetForegroundWindow();
  pd.Flags = PD_RETURNDC | PD_USEDEVMODECOPIESANDCOLLATE | PD_NOSELECTION;
  pd.nMinPage = 1;
  pd.nMaxPage = pagecount;
  BOOL b = PrintDlg (&pd);
  if (pd.hwndOwner) { // restore the correct state of mouse buttons and keyboard modifier keys (STR #3221)
    WNDPROC windproc = (WNDPROC)GetWindowLongPtrW(pd.hwndOwner, GWLP_WNDPROC);
    CallWindowProc(windproc, pd.hwndOwner, WM_ACTIVATEAPP, 1, 0);
  }
  if (b != 0) {
    hPr = pd.hDC;
    if (hPr != NULL) {
      strcpy (docName, "FLTK");
      memset(&di, 0, sizeof(DOCINFO));
      di.cbSize = sizeof (DOCINFO);
      di.lpszDocName = (LPCSTR) docName;
      prerr = StartDoc (hPr, &di);
      if (prerr < 1) {
        abortPrint = TRUE;
        DWORD dw = GetLastError();
        err = (dw == ERROR_CANCELLED ? 1 : 2);
        if (perr_message && err == 2) {
          wchar_t *lpMsgBuf;
          DWORD retval = FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR) &lpMsgBuf,
            0, NULL);
          if (retval) {
            unsigned srclen = lstrlenW(lpMsgBuf);
            while (srclen > 0 && (lpMsgBuf[srclen-1] == '\n' || lpMsgBuf[srclen-1] == '\r')) srclen--;
            unsigned l = fl_utf8fromwc(NULL, 0, lpMsgBuf, srclen);
            *perr_message = new char[l+51];
            sprintf(*perr_message, "begin_job() failed with error %lu: ", dw);
            fl_utf8fromwc(*perr_message + strlen(*perr_message), l+1, lpMsgBuf, srclen);
            LocalFree(lpMsgBuf);
          }
        }
      }
    }
  } else {
    err = 1;
  }
  if(!err) {
    if((pd.Flags & PD_PAGENUMS) != 0 ) {
      if (frompage) *frompage = pd.nFromPage;
      if (topage) *topage = pd.nToPage;
    }
    else {
      if (frompage) *frompage = 1;
      if (topage) *topage = pagecount;
    }
    x_offset = 0;
    y_offset = 0;
    WIN_SetupPrinterDeviceContext (hPr);
    driver()->gc(hPr);
    this->set_current();
#if USE_GDIPLUS
    left_margin = top_margin = printable_height = printable_width = 0;
#endif
  }
  return err;
}

void Fl_WinAPI_Printer_Driver::end_job (void)
{
  Fl_Display_Device::display_device()->set_current();
  if (hPr != NULL) {
    if (! abortPrint) {
      prerr = EndDoc (hPr);
      if (prerr < 0) {
        fl_alert ("EndDoc error %d", prerr);
      }
    }
    DeleteDC (hPr);
    if (pd.hDevMode != NULL) {
      GlobalFree (pd.hDevMode);
    }
    if (pd.hDevNames != NULL) {
      GlobalFree (pd.hDevNames);
    }
  }
  hPr = NULL;
}

#if USE_GDIPLUS
float
#else
void
#endif
    Fl_WinAPI_Printer_Driver::absolute_printable_rect(int *x, int *y, int *w, int *h)
{
  POINT         physPageSize;
  POINT         pixelsPerInch;
#if USE_GDIPLUS
  float f = 1;

  if (hPr == NULL) return f;
  if (printable_width) {
    *x = left_margin; *y = top_margin; *w = printable_width; *h = printable_height;
    return f;
  }
#else
  XFORM         transform;
  
  if (hPr == NULL) return;
  HDC gc = (HDC)driver()->gc();
  GetWorldTransform(gc, &transform);
  ModifyWorldTransform(gc, NULL, MWT_IDENTITY);
  SetWindowOrgEx(gc, 0, 0, NULL);
#endif
  physPageSize.x = GetDeviceCaps(hPr, HORZRES);
  physPageSize.y = GetDeviceCaps(hPr, VERTRES);
#if USE_GDIPLUS
  f = physPageSize.y;
#endif
  DPtoLP(hPr, &physPageSize, 1);
#if USE_GDIPLUS
  f /= physPageSize.y;
#endif
  *w = physPageSize.x + 1;
  *h = physPageSize.y + 1;
  pixelsPerInch.x = GetDeviceCaps(hPr, LOGPIXELSX);
  pixelsPerInch.y = GetDeviceCaps(hPr, LOGPIXELSY);
  DPtoLP(hPr, &pixelsPerInch, 1);
  left_margin = (pixelsPerInch.x / 4);
  *w -= (pixelsPerInch.x / 2);
  top_margin = (pixelsPerInch.y / 4);
  *h -= (pixelsPerInch.y / 2);
#if USE_GDIPLUS
  printable_width = *w; printable_height = *h;
#endif
  *x = left_margin;
  *y = top_margin;
#if USE_GDIPLUS
  return f;
#else
  origin(x_offset, y_offset);
  SetWorldTransform(gc, &transform);
#endif
}

void Fl_WinAPI_Printer_Driver::margins(int *left, int *top, int *right, int *bottom)
{
  int x = 0, y = 0, w = 0, h = 0;
  absolute_printable_rect(&x, &y, &w, &h);
#if USE_GDIPLUS
  if (left) *left = x / scale_x_;
  if (top) *top = y / scale_y_;
  if (right) *right = x / scale_x_;
  if (bottom) *bottom = y / scale_y_;
#else
  if (left) *left = x;
  if (top) *top = y;
  if (right) *right = x;
  if (bottom) *bottom = y;
#endif
}

int Fl_WinAPI_Printer_Driver::printable_rect(int *w, int *h)
{
  int x, y;
  absolute_printable_rect(&x, &y, w, h);
#if USE_GDIPLUS
  *w /= scale_x_; *h /= scale_y_;
#endif
  return 0;
}

int Fl_WinAPI_Printer_Driver::begin_page (void)
{
  int  rsult, w, h;

  rsult = 0;
  if (hPr != NULL) {
    WIN_SetupPrinterDeviceContext (hPr);
    prerr = StartPage (hPr);
    if (prerr < 0) {
      fl_alert ("StartPage error %d", prerr);
      rsult = 1;
    }
#if USE_GDIPLUS
    int x, y;
    float f = absolute_printable_rect(&x, &y, &w, &h);
    ((Fl_GDIplus_Graphics_Driver*)driver())->graphics_ = new Gdiplus::Graphics(hPr);
    ((Fl_GDIplus_Graphics_Driver*)driver())->graphics_->ScaleTransform(f/50, f/50);
    ((Fl_GDIplus_Graphics_Driver*)driver())->graphics_->TranslateTransform(left_margin, top_margin);
    initial_state = ((Fl_GDIplus_Graphics_Driver*)driver())->graphics_->Save();
#else
    printable_rect(&w, &h);
    origin(0, 0);
#endif
    fl_clip_region(0);
  }
  return rsult;
}

void Fl_WinAPI_Printer_Driver::origin (int deltax, int deltay)
{
#if USE_GDIPLUS
  ((Fl_GDIplus_Graphics_Driver*)driver())->graphics_->Restore(initial_state);
  initial_state = ((Fl_GDIplus_Graphics_Driver*)driver())->graphics_->Save();
  ((Fl_GDIplus_Graphics_Driver*)driver())->graphics_->ScaleTransform(scale_x_, scale_y_);
  ((Fl_GDIplus_Graphics_Driver*)driver())->graphics_->TranslateTransform(deltax, deltay);
#else
  SetWindowOrgEx( (HDC)driver()->gc(), - left_margin - deltax, - top_margin - deltay, NULL);
#endif
  x_offset = deltax;
  y_offset = deltay;
}

void Fl_WinAPI_Printer_Driver::scale (float scalex, float scaley)
{
  if (scaley == 0.) scaley = scalex;
#if USE_GDIPLUS
  ((Fl_GDIplus_Graphics_Driver*)driver())->graphics_->Restore(initial_state);
  initial_state = ((Fl_GDIplus_Graphics_Driver*)driver())->graphics_->Save();
  ((Fl_GDIplus_Graphics_Driver*)driver())->graphics_->ScaleTransform(scalex, scaley);
  scale_x_ = scalex; scale_y_ = scaley;
#else
  int w, h;
  SetWindowExtEx((HDC)driver()->gc(), (int)(720 / scalex + 0.5), (int)(720 / scaley + 0.5), NULL);
  printable_rect(&w, &h);
  origin(0, 0);
#endif
}

void Fl_WinAPI_Printer_Driver::rotate (float rot_angle)
{
#if USE_GDIPLUS
  ((Fl_GDIplus_Graphics_Driver*)driver())->graphics_->RotateTransform(-rot_angle);
#else
  XFORM mat;
  float angle;
  angle = (float) - (rot_angle * M_PI / 180.);
  mat.eM11 = (float)cos(angle);
  mat.eM12 = (float)sin(angle);
  mat.eM21 = - mat.eM12;
  mat.eM22 = mat.eM11;
  mat.eDx = mat.eDy = 0;
  SetWorldTransform((HDC)driver()->gc(), &mat);
#endif
}

int Fl_WinAPI_Printer_Driver::end_page (void)
{
  int  rsult;

  rsult = 0;
  if (hPr != NULL) {
#if USE_GDIPLUS
    delete ((Fl_GDIplus_Graphics_Driver*)driver())->graphics_;
    ((Fl_GDIplus_Graphics_Driver*)driver())->graphics_ = NULL;
#endif
    prerr = EndPage (hPr);
    if (prerr < 0) {
      abortPrint = TRUE;
      fl_alert ("EndPage error %d", prerr);
      rsult = 1;
    }
    else { // make sure rotation is not transferred to next page
      ModifyWorldTransform(hPr, NULL, MWT_IDENTITY);
    }
  }
  return rsult;
}

#if !USE_GDIPLUS
static int translate_stack_depth = 0;
const int translate_stack_max = 5;
static int translate_stack_x[translate_stack_max];
static int translate_stack_y[translate_stack_max];

static void do_translate(int x, int y, HDC gc)
{
  XFORM tr;
  tr.eM11 = tr.eM22 = 1;
  tr.eM12 = tr.eM21 = 0;
  tr.eDx =  (FLOAT) x;
  tr.eDy =  (FLOAT) y;
  ModifyWorldTransform(gc, &tr, MWT_LEFTMULTIPLY);
}
#endif

void Fl_WinAPI_Printer_Driver::translate (int x, int y)
{
#if USE_GDIPLUS
  ((Fl_GDIplus_Graphics_Driver*)driver())->translate_all(x, y);
#else
  do_translate(x, y, (HDC)driver()->gc());
  if (translate_stack_depth < translate_stack_max) {
    translate_stack_x[translate_stack_depth] = x;
    translate_stack_y[translate_stack_depth] = y;
    translate_stack_depth++;
    }
#endif
}

void Fl_WinAPI_Printer_Driver::untranslate (void)
{
#if USE_GDIPLUS
  ((Fl_GDIplus_Graphics_Driver*)driver())->untranslate_all();
#else
  if (translate_stack_depth > 0) {
    translate_stack_depth--;
    do_translate( - translate_stack_x[translate_stack_depth], - translate_stack_y[translate_stack_depth], (HDC)driver()->gc() );
    }
#endif
}

void Fl_WinAPI_Printer_Driver::origin(int *x, int *y)
{
  Fl_Paged_Device::origin(x, y);
}
