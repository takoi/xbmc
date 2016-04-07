set(SOURCES LangCodeExpander.cpp
            LangInfo.cpp
            Locale.cpp
            LocalizeStrings.cpp
            POUtils.cpp)

set(HEADERS LangCodeExpander.h
            LangInfo.h
            Locale.h
            LocalizeStrings.h
            POUtils.h)

core_add_library(i18n)
