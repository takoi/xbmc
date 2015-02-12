/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "Repository.h"
#include "addons/AddonDatabase.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/File.h"
#include "filesystem/PluginDirectory.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/JobManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "FileItem.h"
#include "TextureDatabase.h"
#include "URL.h"

using namespace std;
using namespace XFILE;
using namespace ADDON;

AddonPtr CRepository::Clone() const
{
  return AddonPtr(new CRepository(*this));
}

CRepository::CRepository(const AddonProps& props) :
  CAddon(props)
{
}

CRepository::CRepository(const cp_extension_t *ext)
  : CAddon(ext)
{
  // read in the other props that we need
  if (ext)
  {
    AddonVersion version("0.0.0");
    AddonPtr addonver;
    if (CAddonMgr::Get().GetAddon("xbmc.addon", addonver))
      version = addonver->Version();
    for (size_t i = 0; i < ext->configuration->num_children; ++i)
    {
      if(ext->configuration->children[i].name &&
         strcmp(ext->configuration->children[i].name, "dir") == 0)
      {
        AddonVersion min_version(CAddonMgr::Get().GetExtValue(&ext->configuration->children[i], "@minversion"));
        if (min_version <= version)
        {
          DirInfo dir;
          dir.version    = min_version;
          dir.checksum   = CAddonMgr::Get().GetExtValue(&ext->configuration->children[i], "checksum");
          dir.compressed = CAddonMgr::Get().GetExtValue(&ext->configuration->children[i], "info@compressed") == "true";
          dir.info       = CAddonMgr::Get().GetExtValue(&ext->configuration->children[i], "info");
          dir.datadir    = CAddonMgr::Get().GetExtValue(&ext->configuration->children[i], "datadir");
          dir.zipped     = CAddonMgr::Get().GetExtValue(&ext->configuration->children[i], "datadir@zip") == "true";
          dir.hashes     = CAddonMgr::Get().GetExtValue(&ext->configuration->children[i], "hashes") == "true";
          m_dirs.push_back(dir);
        }
      }
    }
    // backward compatibility
    if (!CAddonMgr::Get().GetExtValue(ext->configuration, "info").empty())
    {
      DirInfo info;
      info.checksum   = CAddonMgr::Get().GetExtValue(ext->configuration, "checksum");
      info.compressed = CAddonMgr::Get().GetExtValue(ext->configuration, "info@compressed") == "true";
      info.info       = CAddonMgr::Get().GetExtValue(ext->configuration, "info");
      info.datadir    = CAddonMgr::Get().GetExtValue(ext->configuration, "datadir");
      info.zipped     = CAddonMgr::Get().GetExtValue(ext->configuration, "datadir@zip") == "true";
      info.hashes     = CAddonMgr::Get().GetExtValue(ext->configuration, "hashes") == "true";
      m_dirs.push_back(info);
    }
  }
}

CRepository::CRepository(const CRepository &rhs)
  : CAddon(rhs), m_dirs(rhs.m_dirs)
{
}

CRepository::~CRepository()
{
}

string CRepository::FetchChecksum(const string& url)
{
  CFile file;
  try
  {
    if (file.Open(url))
    {    
      // we intentionally avoid using file.GetLength() for 
      // Transfer-Encoding: chunked servers.
      std::stringstream str;
      char temp[1024];
      int read;
      while ((read=file.Read(temp, sizeof(temp))) > 0)
        str.write(temp, read);
      return str.str();
    }
    return "";
  }
  catch (...)
  {
    return "";
  }
}

string CRepository::GetAddonHash(const AddonPtr& addon) const
{
  string checksum;
  DirList::const_iterator it;
  for (it = m_dirs.begin();it != m_dirs.end(); ++it)
    if (URIUtils::IsInPath(addon->Path(), it->datadir))
      break;
  if (it != m_dirs.end() && it->hashes)
  {
    checksum = FetchChecksum(addon->Path()+".md5");
    size_t pos = checksum.find_first_of(" \n");
    if (pos != string::npos)
      return checksum.substr(0, pos);
  }
  return checksum;
}

#define SET_IF_NOT_EMPTY(x,y) \
  { \
    if (!x.empty()) \
       x = y; \
  }

bool CRepository::Parse(const DirInfo& dir, VECADDONS &result)
{
  string file = dir.info;
  if (dir.compressed)
  {
    CURL url(dir.info);
    string opts = url.GetProtocolOptions();
    if (!opts.empty())
      opts += "&";
    url.SetProtocolOptions(opts+"Encoding=gzip");
    file = url.Get();
  }

  CXBMCTinyXML doc;
  if (doc.LoadFile(file) && doc.RootElement() &&
      CAddonMgr::Get().AddonsFromRepoXML(doc.RootElement(), result))
  {
    for (IVECADDONS i = result.begin(); i != result.end(); ++i)
    {
      AddonPtr addon = *i;
      if (dir.zipped)
      {
        string file = StringUtils::Format("%s/%s-%s.zip", addon->ID().c_str(), addon->ID().c_str(), addon->Version().asString().c_str());
        addon->Props().path = URIUtils::AddFileToFolder(dir.datadir,file);
        SET_IF_NOT_EMPTY(addon->Props().icon,URIUtils::AddFileToFolder(dir.datadir,addon->ID()+"/icon.png"))
        file = StringUtils::Format("%s/changelog-%s.txt", addon->ID().c_str(), addon->Version().asString().c_str());
        SET_IF_NOT_EMPTY(addon->Props().changelog,URIUtils::AddFileToFolder(dir.datadir,file))
        SET_IF_NOT_EMPTY(addon->Props().fanart,URIUtils::AddFileToFolder(dir.datadir,addon->ID()+"/fanart.jpg"))
      }
      else
      {
        addon->Props().path = URIUtils::AddFileToFolder(dir.datadir,addon->ID()+"/");
        SET_IF_NOT_EMPTY(addon->Props().icon,URIUtils::AddFileToFolder(dir.datadir,addon->ID()+"/icon.png"))
        SET_IF_NOT_EMPTY(addon->Props().changelog,URIUtils::AddFileToFolder(dir.datadir,addon->ID()+"/changelog.txt"))
        SET_IF_NOT_EMPTY(addon->Props().fanart,URIUtils::AddFileToFolder(dir.datadir,addon->ID()+"/fanart.jpg"))
      }
    }
    return true;
  }
  return false;
}

void CRepository::OnPostInstall(bool restart, bool update)
{
  CAddonInstaller::Get().ScheduleUpdate();
}

void CRepository::OnPostUnInstall()
{
  CAddonDatabase database;
  database.Open();
  database.DeleteRepository(ID());
}


CRepositoryUpdateJob::CRepositoryUpdateJob(const RepositoryPtr &repo) : m_repo(repo)
{
}


void MergeAddons(map<string, AddonPtr> &addons, const VECADDONS &new_addons)
{
  for (VECADDONS::const_iterator it = new_addons.begin(); it != new_addons.end(); ++it)
  {
    map<string, AddonPtr>::iterator existing = addons.find((*it)->ID());
    if (existing != addons.end())
    { // already got it - replace if we have a newer version
      if (existing->second->Version() < (*it)->Version())
        existing->second = *it;
    }
    else
      addons.insert(make_pair((*it)->ID(), *it));
  }
}


void InvalidateArt(const VECADDONS& addons)
{
  CTextureDatabase textureDB;
  textureDB.Open();
  textureDB.BeginMultipleExecute();

  for (const auto& addon : addons)
  {
   // invalidate the art associated with this item
    if (!addon->Props().fanart.empty())
      textureDB.InvalidateCachedTexture(addon->Props().fanart);
    if (!addon->Props().icon.empty())
      textureDB.InvalidateCachedTexture(addon->Props().icon);
  }

  textureDB.CommitMultipleExecute();
}


void UpdateBrokenStatuses(CAddonDatabase& database, VECADDONS& addons)
{
  auto haveNewer = [&database](const AddonPtr& addon) {
      AddonPtr localAddon;
      return (CAddonMgr::Get().GetAddon(addon->ID(), localAddon) && localAddon->Version() > addon->Version())
        || (database.GetAddonVersion(addon->ID()) > addon->Version());
    };

  // Skip if we have a local version newer that the one in repos
  addons.erase(std::remove_if(addons.begin(), addons.end(), haveNewer), addons.end());

  for (const auto& newAddon : addons)
  {
    if (newAddon->Props().broken.empty() && !CAddonInstaller::Get().CheckDependencies(newAddon, &database))
      newAddon->Props().broken = "DEPSNOTMET";

    bool remoteBroken = !newAddon->Props().broken.empty();
    bool localBroken = !database.IsAddonBroken(newAddon->ID()).empty();
    if (remoteBroken && !localBroken)
    {
      CLog::Log(LOGDEBUG, "RepoUpdater - %s has been marked broken (%s)",
                newAddon->ID().c_str(), newAddon->Props().broken.c_str());
      bool shouldDisable = CGUIDialogYesNo::ShowAndGetInput(newAddon->Name(),
                               g_localizeStrings.Get(newAddon->Props().broken == "DEPSNOTMET" ? 24104 : 24096),
                               g_localizeStrings.Get(24097), "");
      if (shouldDisable)
        CAddonMgr::Get().DisableAddon(newAddon->ID());
    }
    else if (!remoteBroken && localBroken)
    { // Unbreak addon
      database.BreakAddon(newAddon->ID(), "");
      CLog::Log(LOGDEBUG, "RepoUpdater - %s has been unbroken", newAddon->ID().c_str());
    }
  }
}


bool CRepositoryUpdateJob::DoWork()
{
  //Sleep(5000);

  CAddonDatabase database;
  database.Open();
  string oldReposum;
  if (!database.GetRepoChecksum(m_repo->ID(), oldReposum))
    oldReposum = "";

  string reposum;
  for (const auto& dir : m_repo->m_dirs)
  {
    if (ShouldCancel(0, 0))
      return false;

    if (!dir.checksum.empty())
    {
      const string dirsum = CRepository::FetchChecksum(dir.checksum);
      if (dirsum.empty())
      {
        CLog::Log(LOGERROR, "RepoUpdater - Failed to fetch checksum for directory listing %s for repository %s. ",
                  dir.info.c_str(), m_repo->ID().c_str());
        return false;
      }
      reposum += dirsum;
    }
  }

  if (!oldReposum.empty() && oldReposum == reposum)
  {
    CLog::Log(LOGDEBUG, "RepoUpdater - Checksum for repository %s not changed.", m_repo->ID().c_str());
    //database.GetRepository(ID(), addons);
    database.SetRepoTimestamp(m_repo->ID(), CDateTime::GetCurrentDateTime().GetAsDBDateTime());
    return true;
  }

  if (oldReposum != reposum || oldReposum.empty())
  {
    map<string, AddonPtr> uniqueAddons;
    for (const auto& dir : m_repo->m_dirs)
    {
      if (ShouldCancel(0, 0))
        return false;

      VECADDONS addonsInDir;
      if (!CRepository::Parse(dir, addonsInDir))
      { //TODO: Hash is invalid and should not be saved, but should we fail?
        //We can still report a partial addon listing.
        CLog::Log(LOGERROR, "RepoUpdater - Failed to read directory listing %s for repository %s. ",
                  dir.info.c_str(), m_repo->ID().c_str());
        return false;
      }
      MergeAddons(uniqueAddons, addonsInDir);
    }

    VECADDONS addons;
    bool add = true;
    if (!m_repo->Props().libname.empty())
    {
      CFileItemList dummy;
      string s = StringUtils::Format("plugin://%s/?action=update", m_repo->ID().c_str());
      add = CDirectory::GetDirectory(s, dummy);
    }
    if (add)
    {
      for (map<string, AddonPtr>::const_iterator i = uniqueAddons.begin(); i != uniqueAddons.end(); ++i)
        addons.push_back(i->second);
      database.AddRepository(m_repo->ID(), addons, reposum);
      CLog::Log(LOGDEBUG, "RepoUpdater - Repository %s sucessfully updated", m_repo->ID().c_str());
    }

    InvalidateArt(addons);

    //FIXME: this does not work correctly as there may be a newer version in
    //another repo which is not broken. AddRepository should be changed to save
    //broken statuses and disabling should be done in AddonInstaller.
    UpdateBrokenStatuses(database, addons);
  }
  return true;
}

