#pragma once
/*
 *      Copyright (C) 2015 Team XBMC
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

#include <vector>
#include "threads/CriticalSection.h"
#include "utils/RegExp.h"
#include "addons/IAddon.h"
#include "ContextMenuItem.h"
#include "addons/ContextMenuAddon.h"
#include "dialogs/GUIDialogContextMenu.h"


class CURLResolver
{
public:
  CURLResolver();
  bool Resolve(const std::string& url, CFileItem& result);
  bool Register(const ADDON::PluginPtr& addon);
  static CURLResolver& GetInstance();
  virtual ~CURLResolver() {};

private:
  void Init();
  CURLResolver(const CURLResolver&) = delete;
  CURLResolver(CURLResolver&&) = default;
  CURLResolver& operator=(const CURLResolver&) = delete;
  CURLResolver& operator=(CURLResolver&&) = default;

  std::vector<std::pair<CRegExp, std::string>> m_exprs;
  CCriticalSection m_criticalSection;
};
