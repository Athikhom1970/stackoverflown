CONFIG += console c++14
CONFIG -= app_bundle qt
DEFINES += \
  QT_DEPRECATED_WARNINGS \
  QT_DISABLE_DEPRECATED_BEFORE=0x060000 \
  QT_RESTRICTED_CAST_FROM_ASCII QT_NO_FOREACH
SOURCES = main.cpp
