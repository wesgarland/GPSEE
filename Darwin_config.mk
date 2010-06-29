# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Initial Developer of the Original Code is PageMail, Inc.
#
# Portions created by the Initial Developer are 
# Copyright (c) 2007-2009, PageMail, Inc. All Rights Reserved.
#
# Contributor(s):
# 
# Alternatively, the contents of this file may be used under the terms of
# either of the GNU General Public License Version 2 or later (the "GPL"),
# or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK ***** 
#

DEFAULT_GPSEE_PREFIX_DIR = /opt/local/gpsee
LEADING_CPPFLAGS	+= -D_GNU_SOURCE
SOLIB_EXT	 	 = dylib
LDFLAGS_SOLIB_DIRS	 = $(foreach DIR, $(SOLIB_DIRS), -L$(DIR))
LINKER		 	 = $(GCC) -dynamiclib $(JSAPI_LIBS) -undefined dynamic_lookup
CC			 = $(GCC)

GPSEE_C_DEFINES         += HAVE_NDBM
lib%.dylib: LDFLAGS += -Wl,-install_name,$(SOLIB_DIR)/$@      

clean: DEBUG_DUMP_DIRS += $(wildcard $(foreach PROG, $(notdir $(PROGS)), ./$(PROG).dSYM))

# Prefer Apple iconv over MacPorts/Fink
ICONV_LDFLAGS   	= /usr/lib/libiconv.dylib
ICONV_HEADER    	= /usr/include/iconv.h

# Prefer Apple curl over MacPorts/Fink
CURL_LDFLAGS		?= -lcurl
CURL_CPPFLAGS		?= 
