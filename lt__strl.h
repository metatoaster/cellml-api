/* lt__strl.h -- size-bounded string copying and concatenation
   Copyright (C) 2004 Free Software Foundation, Inc.
   Written by Bob Friesenhahn <bfriesen@simple.dallas.tx.us>

   NOTE: The canonical source of this file is maintained with the
   GNU Libtool package.  Report bugs to bug-libtool@gnu.org.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

As a special exception to the GNU Lesser General Public License,
if you distribute this file as part of a program or library that
is built using GNU libtool, you may include it under the same
distribution terms that you use for the rest of that program.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301  USA

*/

#if !defined(LT__STRL_H)
#define LT__STRL_H 1

#if defined(HAVE_CONFIG_H)
#  if defined(LT_CONFIG_H)
#    include LT_CONFIG_H
#  else
#    include <config.h>
#  endif
#endif

#include <string.h>
#include "libltdl/libltdl/lt_system.h"

#if !defined(HAVE_STRLCAT)
#  define strlcat(dst,src,dstsize) lt_strlcat(dst,src,dstsize)
LT_SCOPE size_t lt_strlcat(char *dst, const char *src, const size_t dstsize);
#endif /* !defined(HAVE_STRLCAT) */

#if !defined(HAVE_STRLCPY)
#  define strlcpy(dst,src,dstsize) lt_strlcpy(dst,src,dstsize)
LT_SCOPE size_t lt_strlcpy(char *dst, const char *src, const size_t dstsize);
#endif /* !defined(HAVE_STRLCPY) */

#endif /*!defined(LT__STRL_H)*/