SRCS=	\
  TestLangCodeExpander.cpp \
  TestLocale.cpp \
  TestPOUtils.cpp \

LIB=i18nTest.a

INCLUDES += -I../../../lib/gtest/include

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
