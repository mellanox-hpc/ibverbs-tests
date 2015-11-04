ACLOCAL_AMFLAGS = -I config
AM_CXXFLAGS = -g -Wall -Wstrict-aliasing=0 -Werror -Isrc

bin_PROGRAMS = out/ibv_test

out_ibv_test_SOURCES = src/main.cc src/common.cc src/common.h 
out_ibv_test_SOURCES += src/gtest/gtest.h
out_ibv_test_SOURCES += src/general/loopback.cc

EXTRA_DIST = autogen.sh
