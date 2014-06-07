#ifndef BSEARCH_H
/*
 *      Copyright (C) 2014 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#define BSEARCH_H

#include <cstdlib>
#include <climits>
#include <cassert>

template<typename T, size_t N>
ssize_t static_bsearch(const T (&a)[N], const T& wc)
{
  assert(a);
  assert(N >= 0);
  assert(N <= SSIZE_MAX);

  ssize_t low     = 0;
  ssize_t high    = N - 1;
  ssize_t needle;
  ssize_t retval  = -1;

  while (low <= high)
  {
    needle = low + ((high - low) / 2);
    if (wc == a[needle])
    {
      retval = needle;
      break;
    }
    else if (wc < a[needle])
      high  = needle - 1;
    else
      low = needle + 1;
  }

  return retval;
}

#endif // BSEARCH_H
