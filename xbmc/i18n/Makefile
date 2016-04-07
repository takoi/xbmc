SRCS=LangCodeExpander.cpp \
     LangInfo.cpp \
     Locale.cpp \
     LocalizeStrings.cpp \
     POUtils.cpp \

LIB=i18n.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))