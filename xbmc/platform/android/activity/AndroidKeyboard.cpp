/*
 *      Copyright (C) 2017 Chris Browet
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AndroidKeyboard.h"

#include "utils/log.h"
#include "platform/android/activity/XBMCApp.h"

CAndroidKeyboard::CAndroidKeyboard()
  : m_isCanceled(false)
{
}

bool CAndroidKeyboard::ShowAndGetInput(char_callback_t pCallback, const std::string& initialString, std::string& typedString, const std::string& heading, bool bHiddenInput)
{
  bool ret = CXBMCApp::get()->ShowKeyboard(heading, initialString);
  if (!ret)
    return false;

  std::string res = CXBMCApp::get()->WaitForKeyboard();
  CLog::Log(LOGDEBUG, "%s - %s", __PRETTY_FUNCTION__, res.c_str());

  if (res == "@@-1@@")
  {
    res = "";
    m_isCanceled = true;
  }
//  pCallback(this, res);
  typedString = res;

  return !m_isCanceled;
}

void CAndroidKeyboard::Cancel()
{
  m_isCanceled = true;
  CXBMCApp::get()->CancelKeyboard();
}

bool CAndroidKeyboard::SetTextToKeyboard(const std::string& text, bool closeKeyboard)
{
  CXBMCApp::get()->SetKeyboardText(text);
  if (closeKeyboard)
    CXBMCApp::get()->CancelKeyboard();
}
