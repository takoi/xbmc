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

#include <memory>
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIWindowManager.h"
#include "URLResolver.h"
#include "addons/AddonManager.h"
#include "addons/PluginSource.h"
#include "FileItem.h"
#include "URL.h"
#include "filesystem/PluginDirectory.h"
#include "utils/URIUtils.h"


CURLResolver::CURLResolver()
{
  Init();
}

CURLResolver& CURLResolver::GetInstance()
{
  static CURLResolver instance;
  return instance;
}

void CURLResolver::Init()
{
  CSingleLock lock(m_criticalSection);
  using namespace ADDON;

  VECADDONS addons;
  if (CAddonMgr::GetInstance().GetAddons(ADDON_PLUGIN, addons))
  {
    for (const auto& addon : addons)
    {
      PluginPtr plugin = std::static_pointer_cast<CPluginSource>(addon);
      if (plugin->Provides(CPluginSource::VIDEO) && !plugin->m_resolverRegex.empty())
      {
        CRegExp expr;
        if (expr.RegComp(plugin->m_resolverRegex))
        {
          m_exprs.push_back(std::make_pair(expr, plugin->ID()));
        }
        else
        {

        }
      }
    }
  }
}

bool CURLResolver::Register(const ADDON::PluginPtr& addon)
{
  return false;
}

bool CURLResolver::Resolve(const std::string& path, CFileItem& result)
{
  std::vector<std::string> addons;
  {
    CSingleLock lock(m_criticalSection);
    for (auto& exprId : m_exprs)
    {
      if (exprId.first.RegFind(path) >= 0)
      {
        addons.push_back(exprId.second);
      }
    }
  }

  if (addons.empty())
    return false;

  std::string selected = addons.front();
  if (addons.size() > 1)
  {
    auto* dialog = static_cast<CGUIDialogSelect*>(g_windowManager.GetWindow(WINDOW_DIALOG_SELECT));
    dialog->Reset();
    for (const auto& id : addons)
      dialog->Add(id);
    dialog->Open();
    if (!dialog->IsConfirmed())
      return false;
    selected = addons.at(dialog->GetSelectedLabel());
  }

  CURL url;
  url.SetProtocol("plugin");
  url.SetHostName(selected);
  url.SetFileName("resolve");
  url.SetOption("url", path);
  return XFILE::CPluginDirectory::GetPluginResult(url.Get(), result);
}