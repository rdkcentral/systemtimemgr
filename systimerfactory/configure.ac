# Copyright 2023 Comcast Cable Communications Management, LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
#
#                                              -*- Autoconf -*-
# Process this file with autoconf to produce configure script.
#

AC_PREREQ([2.65])
AC_INIT([nlmon], [1.0], [balaji_punnuru@cable.comcast.com])
AM_INIT_AUTOMAKE([foreign])
LT_INIT

AC_PREFIX_DEFAULT(`pwd`)
AC_ENABLE_SHARED
AC_DISABLE_STATIC

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AM_PROG_LIBTOOL(libtool)

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

DEBUG_CXXFLAGS=" "

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC

AC_ARG_ENABLE([debug],
        AS_HELP_STRING([--enable-debug],[enable debug build]),
        [
          case "${enableval}" in
           yes) DEBUG_CXXFLAGS="-D_DEBUG_";;
           no) DEBUG_CXXFLAGS=" ";;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-headless ]) ;;
           esac
           ],
         [echo "Debug is Disabled."])

AC_ARG_ENABLE([unittest],
        AS_HELP_STRING([--enable-unittest],[enable unittest build]),
        [
          case "${enableval}" in
           yes) UNITTEST_CXXFLAGS="-D__LOCAL_TEST_";;
           no) UNITTEST_CXXFLAGS=" ";;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-unittest ]) ;;
           esac
           ],
         [echo "unittest is disabled"])

IARM_LIBS=" "
IARM_CFLAGS=" "
IARM_ENABLED=""
PWRMGR_CFLAGS=" "
PWRMGRPLUGIN_ENABLED=""
TEE_CXXFLAGS=" "
WPEVGDRM_CXXFLAGS=" "
DTT_ENABLED=""
DTT_CXXFLAGS=" "
AC_ARG_ENABLE([iarm],
        AS_HELP_STRING([--enable-iarm],[enable iarm ]),
        [
          case "${enableval}" in
           yes) IARM_LIBS=" -ldbus-1  -lIARMBus"
                IARM_CFLAGS="-DENABLE_IARM"
                IARM_ENABLED=true;;
           no) IARM_ENABLED=false;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-iarm ]) ;;
           esac
           ],
         [echo "iarm build is enable"])
#The pwrmgrplugin is used for either iarmpowersubscriber or powercontroller
#The timerstatussubscriber uses the IARMBus and dbus libaries.
AC_ARG_ENABLE([pwrmgrplugin],
        AS_HELP_STRING([--enable-pwrmgrplugin],[enable pwrmgrplugin ]),
        [
          case "${enableval}" in
           yes) IARM_LIBS=" -ldbus-1  -lIARMBus"
                PWRMGR_CFLAGS="-DPWRMGRPLUGIN_ENABLED"
                PWRMGRPLUGIN_ENABLED=true;;
           no) PWRMGRPLUGIN_ENABLED=false;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-pwrmgrplugin ]) ;;
           esac
           ],
         [echo "pwrmgrplugin build is enable"])


AC_ARG_ENABLE([dtt],
        AS_HELP_STRING([--enable-dtt],[enable dtt ]),
        [
          case "${enableval}" in
           yes) DTT_CXXFLAGS="-DDTT_ENABLED"
		DTT_ENABLED=true;;
           no) DTT_ENABLED=false;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-dtt ]) ;;
           esac
           ],
         [echo "dtt build is enable"])

AC_ARG_ENABLE([tee],
        AS_HELP_STRING([--enable-tee],[enable tee ]),
        [
          case "${enableval}" in
           yes) TEE_CXXFLAGS="-DTEE_ENABLED -I${PKG_CONFIG_SYSROOT_DIR}${includedir}/rdk/iarmmgrs"
                TEE_ENABLED=true;;
           no) TEE_ENABLED=false;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-iarm ]) ;;
           esac
           ],
         [echo "iarm build is enable"])

AC_ARG_ENABLE([wpevgdrm],
        AS_HELP_STRING([--enable-wpevgdrm],[enable wpevgdrm ]),
        [
          case "${enableval}" in
           yes) WPEVGDRM_CXXFLAGS="-DWPEVGDRM_ENABLED"
		PKG_CHECK_MODULES([WPEFRAMEWORKCORE],[WPEFrameworkCore >= 1.0.0])
		PKG_CHECK_MODULES([WPEFRAMEWORKWEBSOCKET],[WPEFrameworkWebSocket >= 1.0.0])
                WPEVGDRM_ENABLED=true;;
           no) WPEVGDRM_ENABLED=false;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-wpevgdrm ]) ;;
           esac
           ],
         [echo "wpevgdrm build is enable"])

AM_CONDITIONAL([IARM_ENABLED], [test x$IARM_ENABLED = xtrue])
AM_CONDITIONAL([PWRMGRPLUGIN_ENABLED], [test x$PWRMGRPLUGIN_ENABLED = xtrue])
AM_CONDITIONAL([DTT_ENABLED], [test x$DTT_ENABLED = xtrue])
AM_CONDITIONAL([TEE_ENABLED], [test x$TEE_ENABLED = xtrue])
PKG_CHECK_MODULES([JSONRPC],[libjsonrpccpp-client >= 0.7.0])

AC_CONFIG_FILES(Makefile)
AC_SUBST(DEBUG_CXXFLAGS)
AC_SUBST(UNITTEST_CXXFLAGS)
AC_SUBST(IARM_LIBS)
AC_SUBST(IARM_CFLAGS)
AC_SUBST(TEE_CXXFLAGS)
AC_SUBST(WPEVGDRM_CXXFLAGS)
AC_SUBST(DTT_CXXFLAGS)


AC_OUTPUT

