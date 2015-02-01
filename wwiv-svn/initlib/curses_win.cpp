/**************************************************************************/
/*                                                                        */
/*                 WWIV Initialization Utility Version 5.0                */
/*             Copyright (C)1998-2015, WWIV Software Services             */
/*                                                                        */
/*    Licensed  under the  Apache License, Version  2.0 (the "License");  */
/*    you may not use this  file  except in compliance with the License.  */
/*    You may obtain a copy of the License at                             */
/*                                                                        */
/*                http://www.apache.org/licenses/LICENSE-2.0              */
/*                                                                        */
/*    Unless  required  by  applicable  law  or agreed to  in  writing,   */
/*    software  distributed  under  the  License  is  distributed on an   */
/*    "AS IS"  BASIS, WITHOUT  WARRANTIES  OR  CONDITIONS OF ANY  KIND,   */
/*    either  express  or implied.  See  the  License for  the specific   */
/*    language governing permissions and limitations under the License.   */
/*                                                                        */
/**************************************************************************/

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include "curses.h"
#include "core/strings.h"
#include "initlib/curses_win.h"

using std::string;
using wwiv::strings::StringPrintf;

CursesWindow::CursesWindow(CursesWindow* parent, ColorScheme* color_scheme, int nlines, int ncols, int begin_y, int begin_x) 
    : parent_(parent), color_scheme_(color_scheme), current_scheme_id_(SchemeId::UNKNOWN) {
  if (parent != nullptr) {
    if (begin_x == -1) {
      begin_x = (parent->GetMaxX() - ncols) / 2;
    }
    if (begin_y == -1) {
      begin_y = (parent->GetMaxY() - nlines) / 2;
    }
    window_ =  newwin(nlines, ncols, begin_y + getbegy(parent->window()), 
      begin_x + getbegx(parent->window()));
  } else {
    if (begin_x == -1) { begin_x = 0; }
    if (begin_y == -1) { begin_y = 0; }
    window_ = newwin(nlines, ncols, begin_y, begin_x);
  }
  keypad(window_, true);
  werase(window_);
  touchwin(window_);
  Refresh();
}

CursesWindow::~CursesWindow() {
  delwin(window_);
  if (parent_) {
    parent_->RedrawWin();
    parent_->Refresh();
    doupdate();
  }
}

void CursesWindow::SetTitle(const std::string& title) {
  SchemeId saved_scheme(this->current_scheme_id());
  SetColor(SchemeId::WINDOW_TITLE);
  int max_title_size = GetMaxX() - 6;
  string working_title(StringPrintf(" %s ", title.c_str()));
  if (static_cast<int>(title.size()) > max_title_size) {
    working_title = " ";
    working_title += title.substr(0, max_title_size);
    working_title += " ";
  }

  int x = GetMaxX() - 1 - working_title.size();
  PutsXY(x, 0, working_title);
  SetColor(saved_scheme);
}

int CursesWindow::GetChar() const {
  for (;;) {
    int ch = wgetch(window_);
    if (ch == KEY_RESIZE) {
      // Since we don't support online window resizing just ignore this
      // but don't return it as a key for the application to (badly) handle.
      continue;
    }
    return ch;
  }
}

void CursesWindow::GotoXY(int x, int y) {
  x = std::max<int>(x, 0);
  x = std::min<int>(x, GetMaxX());
  y = std::max<int>(y, 0);
  y = std::min<int>(y, GetMaxY());

  Move(y, x);
  Refresh();
}

void CursesWindow::Putch(unsigned char ch) {
  AddCh(ch);
  Refresh();
}

void CursesWindow::Puts(const std::string& text) {
  AddStr(text.c_str());
  Refresh();
}

void CursesWindow::PutsXY(int x, int y, const std::string& text) {
  GotoXY(x, y);
  Puts(text.c_str());
}

/**
 * Printf sytle output function.  Most init output code should use this.
 */
void CursesWindow::Printf(const char* format, ...) {
  va_list ap;
  char szBuffer[1024];

  va_start(ap, format);
  vsnprintf(szBuffer, 1024, format, ap);
  va_end(ap);
  Puts(szBuffer);
}

void CursesWindow::PrintfXY(int x, int y, const char* format, ...) {
  va_list ap;
  char szBuffer[1024];

  va_start(ap, format);
  vsnprintf(szBuffer, 1024, format, ap);
  va_end(ap);
  PutsXY(x, y, szBuffer);
}

void CursesWindow::SetColor(SchemeId id) {
  AttrSet(color_scheme_->GetAttributesForScheme(id));
  current_scheme_id_ = id;
}
