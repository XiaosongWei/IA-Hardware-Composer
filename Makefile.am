#
# Copyright (c) 2016 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

include Makefile.sources

ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

AM_MAKEFLAGS = -s

SUBDIRS = \
	common \
	wsi \
	. \
	tests/third_party/json-c \
	tests

MAINTAINERCLEANFILES = ChangeLog INSTALL

AM_CPP_INCLUDES = -I$(top_srcdir) -Ipublic -Ios -Ios/linux -Icommon/display/ -Icommon/core/ -Icommon/utils/ -Icommon/compositor/ -Iwsi -Iwsi/drm/
AM_CPPFLAGS = -std=c++11 -fPIC -O2 -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fPIE -DENABLE_DOUBLE_BUFFERING -DENABLE_IMPLICIT_CLONE_MODE
AM_CPPFLAGS += $(AM_CPP_INCLUDES) $(CWARNFLAGS) $(DRM_CFLAGS) $(DEBUG_CFLAGS) -Wformat -Wformat-security

if !ENABLE_GBM
AM_CPPFLAGS += -DUSE_MINIGBM
endif

libhwcomposer_la_LIBADD = \
	$(DRM_LIBS) \
	$(GBM_LIBS) \
	$(EGL_LIBS) \
        $(top_builddir)/common/libhwcomposer_common.la \
        $(top_builddir)/wsi/libhwcomposer_wsi.la \
	-lm

libhwcomposer_la_LTLIBRARIES = libhwcomposer.la
libhwcomposer_la_SOURCES = $(hwc_SOURCES)
libhwcomposer_ladir = $(libdir)
libhwcomposer_la_LDFLAGS = -version-number 0:0:1 -no-undefined -shared -Wl,-whole-archive,common/.libs/libhwcomposer_common.a,wsi/.libs/libhwcomposer_wsi.a,-no-whole-archive

if ENABLE_VULKAN
AM_CPP_INCLUDES += -Icommon/compositor/vk
AM_CPPFLAGS += -Icommon/compositor/vk -DUSE_VK -DDISABLE_EXPLICIT_SYNC
libhwcomposer_la_LDFLAGS += -Wl,--no-as-needed,-lvulkan,--as-needed
else
AM_CPP_INCLUDES += -Icommon/compositor/gl
AM_CPPFLAGS += -DUSE_GL
libhwcomposer_la_LIBADD += $(GLES2_LIBS)
endif


libhwcincdir = $(includedir)/libhwc
libhwcinc_HEADERS =	\
	$(top_srcdir)/public/gpudevice.h	\
	$(top_srcdir)/public/hwcbuffer.h	\
	$(top_srcdir)/public/hwcdefs.h	\
	$(top_srcdir)/public/hwclayer.h	\
	$(top_srcdir)/public/hwcrect.h	\
	$(top_srcdir)/public/nativebufferhandler.h	\
	$(top_srcdir)/public/nativedisplay.h	\
	$(top_srcdir)/public/spinlock.h

.PHONY: ChangeLog INSTALL

INSTALL:
	$(INSTALL_CMD)

ChangeLog:
	$(CHANGELOG_CMD)

dist-hook: ChangeLog INSTALL
