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
TEE_CXXFLAGS=" "
DTT_CXXFLAGS=" "

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
         [echo "debug build is enable"])

AC_ARG_ENABLE([tee],
        AS_HELP_STRING([--enable-tee],[enable tee build]),
        [
          case "${enableval}" in
           yes) TEE_CXXFLAGS="-DTEE_ENABLED";;
           no) TEE_CXXFLAGS=" ";;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-tee ]) ;;
           esac
           ],
         [echo "Tee build is enable"])

AC_ARG_ENABLE([dtt],
        AS_HELP_STRING([--enable-dtt],[enable dtt build]),
        [
          case "${enableval}" in
           yes) DTT_CXXFLAGS="-DDTT_ENABLED";;
           no) DTT_CXXFLAGS=" ";;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-tee ]) ;;
           esac
           ],
         [echo "Tee build is enable"])

AC_ARG_ENABLE([t2api],
        AS_HELP_STRING([--enable-t2api],[enables telemetry]),
        [
          case "${enableval}" in
           yes) IS_TELEMETRY2_ENABLED=true
                T2_EVENT_FLAG=" -DT2_EVENT_ENABLED ";;
           no)  IS_TELEMETRY2_ENABLED=false ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-t2enable]) ;;
           esac
           ],
        [echo "telemetry is disabled"])
AM_CONDITIONAL([IS_TELEMETRY2_ENABLED], [test x$IS_TELEMETRY2_ENABLED = xtrue])
AC_SUBST(T2_EVENT_FLAG)

AC_CONFIG_FILES(Makefile)
AC_SUBST(DEBUG_CXXFLAGS)
AC_SUBST(TEE_CXXFLAGS)
AC_SUBST(DTT_CXXFLAGS)


AC_OUTPUT

