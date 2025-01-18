
set (JLSIGNAL_SOURCES
    ObjectPool.cpp
    SignalBase.cpp
)

add_library(jlsignal ${JLSIGNAL_SOURCES})

target_include_directories(jlsignal PUBLIC ./)



