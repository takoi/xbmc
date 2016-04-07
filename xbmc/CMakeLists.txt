set(SOURCES Application.cpp
            ApplicationPlayer.cpp
            AppParamParser.cpp
            AutoSwitch.cpp
            BackgroundInfoLoader.cpp
            DatabaseManager.cpp
            DbUrl.cpp
            DynamicDll.cpp
            FileItem.cpp
            FileItemListModification.cpp
            GUIInfoManager.cpp
            GUILargeTextureManager.cpp
            GUIPassword.cpp
            MediaSource.cpp
            PasswordManager.cpp
            PlayListPlayer.cpp
            PartyModeManager.cpp
            SectionLoader.cpp
            ServiceBroker.cpp
            ServiceManager.cpp
            SystemGlobals.cpp
            TextureCache.cpp
            TextureCacheJob.cpp
            TextureDatabase.cpp
            ThumbLoader.cpp
            URL.cpp
            Util.cpp
            XBApplicationEx.cpp)

set(HEADERS AppParamParser.h
            Application.h
            ApplicationPlayer.h
            AutoSwitch.h
            BackgroundInfoLoader.h
            CompileInfo.h
            DatabaseManager.h
            DbUrl.h
            DllPaths.h
            DllPaths_win32.h
            DynamicDll.h
            FileItem.h
            FileItemListModification.h
            GUIInfoManager.h
            GUILargeTextureManager.h
            GUIPassword.h
            GUIUserMessages.h
            IFileItemListModifier.h
            IProgressCallback.h
            MediaSource.h
            PartyModeManager.h
            PasswordManager.h
            PlayListPlayer.h
            SectionLoader.h
            ServiceBroker.h
            ServiceManager.h
            TextureCache.h
            TextureCacheJob.h
            TextureDatabase.h
            ThumbLoader.h
            URL.h
            Util.h
            XBApplicationEx.h
            system.h
            system_gl.h)

core_add_library(xbmc)
add_dependencies(xbmc libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(xbmc ffmpeg)
endif()
