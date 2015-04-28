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

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif

// python.h should always be included first before any other includes
#include <Python.h>
#include <osdefs.h>

#include "system.h"
#include "AddonRepoInvoker.h"
#include "addons/AddonManager.h"
#include "addons/Repository.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "interfaces/legacy/Addon.h"
#include "interfaces/python/swig.h"
#include "utils/log.h"
#include "utils/URIUtils.h"


CAddonRepoInvoker::CAddonRepoInvoker(
    ILanguageInvocationHandler *invocationHandler)
  : CAddonPythonInvoker(invocationHandler), m_invoked(false)
{
}

CAddonRepoInvoker::~CAddonRepoInvoker()
{
}

std::string CAddonRepoInvoker::GetChecksum()
{
  if (!m_invoked)
  {
    m_invoked = true;
    Execute(m_addon->LibPath());
  }
  return m_checksum;
}

std::vector<ADDON::AddonPtr> CAddonRepoInvoker::GetAddons()
{
  if (!m_invoked)
  {
    m_invoked = true;
    Execute(m_addon->LibPath());
  }
  return m_addons;
}

void CAddonRepoInvoker::executeScript(void *fp, const std::string &script, void *module, void *moduleDict)
{
  assert(m_addon != nullptr);
  auto repo = std::static_pointer_cast<ADDON::CRepository>(m_addon);

  std::string moduleName = URIUtils::GetFileName(m_addon->LibPath());
  URIUtils::RemoveExtension(moduleName);
  const std::string entryPoint = repo->GetEntryPoint();

  PyObject* pyModule = NULL;
  PyObject* pyCallable = NULL;
  PyObject* pyGenerator = NULL;
  PyObject* pyResultIterator = NULL;
  PyObject* pyIterResult = NULL;

  pyModule = PyImport_ImportModule(moduleName.c_str());
  if (pyModule == NULL)
  {
    CLog::Log(LOGERROR, "CAddonRepoInvoker: failed to import module");
    goto cleanup;
  }

  pyCallable = PyObject_GetAttrString(pyModule, entryPoint.c_str());
  if (pyCallable == NULL || !PyCallable_Check(pyCallable))
  {
    CLog::Log(LOGERROR, "CAddonRepoInvoker: failed get callable");
    goto cleanup;
  }


  pyGenerator = PyObject_CallObject(pyCallable, NULL);
  Py_DECREF(pyCallable);
  if (pyGenerator == NULL)
  {
    CLog::Log(LOGERROR, "CAddonRepoInvoker: failed to call object");
    goto cleanup;
  }

  pyResultIterator = PyObject_GetIter(pyGenerator);
  Py_DECREF(pyGenerator);
  if (pyResultIterator == NULL || !PyIter_Check(pyResultIterator))
  {
    CLog::Log(LOGERROR, "CAddonRepoInvoker: not iterator");
    goto cleanup;
  }

  pyIterResult = PyIter_Next(pyResultIterator);
  if (pyIterResult == NULL)
  {
    CLog::Log(LOGERROR, "CAddonRepoInvoker: generator did not return");
    goto cleanup;
  }

  try
  {
    PythonBindings::PyXBMCGetUnicodeString(m_checksum, pyIterResult);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CAddonRepoInvoker: exception while parsing checksum");
    goto cleanup;
  }

  while ((pyIterResult = PyIter_Next(pyResultIterator)) != NULL)
  {
    XBMCAddon::xbmcaddon::AddonProps* props;
    try
    {
      props = (XBMCAddon::xbmcaddon::AddonProps*)PythonBindings::retrieveApiInstance(
        pyIterResult,"p.XBMCAddon::xbmcaddon::AddonProps", "XBMCAddon::xbmcaddon::", "");
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "CAddonRepoInvoker: unexpected return from generator");
      goto cleanup;
    }

    assert(props != nullptr);
    ADDON::AddonProps internal(props->id, ADDON::TranslateType(props->addonType), props->version, "");
    internal.name = props->name;
    ADDON::AddonPtr addon = ADDON::CAddonMgr::Get().AddonFromProps(internal);

    //TODO: assert(addon.get() != nullptr);
    if (addon)
      m_addons.push_back(addon);

    Py_DECREF(pyIterResult);
  }

cleanup:
  if (pyIterResult != NULL)
  {
    Py_DECREF(pyIterResult);
  }
  if (pyResultIterator != NULL)
  {
    Py_DECREF(pyResultIterator);
  }
  if (pyModule != NULL)
  {
    Py_DECREF(pyModule);
  }
  return;
}
