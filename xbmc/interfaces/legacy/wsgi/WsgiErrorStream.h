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

#include "interfaces/legacy/AddonClass.h"
#include "interfaces/legacy/swighelper.h"

struct HTTPPythonRequest;

namespace XBMCAddon
{
  namespace xbmcwsgi
  {
    /**
     * TODO
     */
    class WsgiErrorStream : public AddonClass
    {
    public:
      WsgiErrorStream();
      virtual ~WsgiErrorStream();

      /**
       * TODO
       */
      inline void flush() { }

      /**
       * TODO
       */
      void write(const String& str);

      /**
       * TODO
       */
      void writelines(const std::vector<String>& seq);

#ifndef SWIG
      /**
       * Sets the given request.
       */
      void SetRequest(HTTPPythonRequest* request);

      HTTPPythonRequest* m_request;
#endif
    };    
  }
}


