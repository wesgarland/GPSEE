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
ICONV_LDFLAGS		 =
GPSEE_C_DEFINES		+= HAVE_MEMRCHR
GPSEE_C_DEFINES		+= HAVE_IDENTITY_TRANSCODING_ICONV
EXTRA_CPPFLAGS		+= -D_GNU_SOURCE
GFFI_CPPFLAGS		?= -D_GNU_SOURCE -DDB_DBM_HSEARCH=1

# Some GNU/Linux distributions require extra packages to be installed for the SUSv3 NDBM API
# Include this in your local_config.mk if you have them:
#GFFI_LDFLAGS += -ldb
#GPSEE_C_DEFINES += HAVE_NDBM

# Where is cURL? Over-ride with local_config.mk if necessary
ifneq $(X,X$(wildcard /usr/local/include/curl/curl.h))
CURL_LDFLAGS    ?= -L/usr/local/lib -R/opt/sfw/lib -lcurl
CURL_CPPFLAGS   ?= -I/usr/local/include
endif

