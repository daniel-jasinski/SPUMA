/*---------------------------------------------------------------------------*\
  *      .  *_______ * ______ .  __ *  __ * ___ .___    .  ___ .   *  .     *
    *  .    /       | |   _  \  |  |  |  | |   \/   | *   /   \ *   .    *   .
 *    .  * .\   (---*.|  |_)  |.|  |  |  |*|  \  /  |. * /  *  \  .  *     *
 =^^=^^==^^^=\   \^=^=|   ___/=^|  |^=|  |=|  |\/|  |^^=/  /=\  \^=^=^^===^^^=
 0  o  O  o---)   \ 0 |  |   0  |  o--o  |o|  |  |  | o/  _____  \ 0   o  O
     0    |_______/   |__| o   o \______/  |__| 0|__| /__/  o  \__\   o
  O   o  o        0  o      0   O        o    o       O  o     0   o    0  o
-------------------------------------------------------------------------------
    Copyright (C) 2025 Cineca
-------------------------------------------------------------------------------
License
    This file is part of SPUMA.

    SPUMA is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SPUMA is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with SPUMA.  If not, see <http://www.gnu.org/licenses/>.

Description
    Warp reduce utility functions for HIP reductions.

SourceFiles
    warpReduce.hpp

\*---------------------------------------------------------------------------*/

#ifndef Foam_warpReduce_hpp
#define Foam_warpReduce_hpp

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

namespace hip
{

template <typename T>
__device__
void warpReduceNoVolatile( T* sdata, const unsigned int tid, const unsigned int blockSize)
{
    T tmp;
    if (blockSize >= 64) { tmp = sdata[tid + 32];  __threadfence_block(); sdata[tid]+=tmp; __threadfence_block(); }
    if (blockSize >= 32) { tmp = sdata[tid + 16];  __threadfence_block(); sdata[tid]+=tmp; __threadfence_block(); }
    if (blockSize >= 16) { tmp = sdata[tid +  8];  __threadfence_block(); sdata[tid]+=tmp; __threadfence_block(); }
    if (blockSize >=  8) { tmp = sdata[tid +  4];  __threadfence_block(); sdata[tid]+=tmp; __threadfence_block(); }
    if (blockSize >=  4) { tmp = sdata[tid +  2];  __threadfence_block(); sdata[tid]+=tmp; __threadfence_block(); }
    if (blockSize >=  2) { tmp = sdata[tid +  1];  __threadfence_block(); sdata[tid]+=tmp; __threadfence_block(); }
};

template <typename T,typename Op>
__device__
void warpReduceCompareNoVolatile( T* sdata, Op& op, const unsigned int tid, const unsigned int blockSize)
{
    T tmp;
    if (blockSize >= 64) { tmp = op(sdata[tid],sdata[tid + 32]);__threadfence_block();
        sdata[tid]= tmp;  __threadfence_block(); }
    if (blockSize >= 32) { tmp = op(sdata[tid],sdata[tid + 16]);__threadfence_block();
        sdata[tid]= tmp;  __threadfence_block(); }
    if (blockSize >= 16) { tmp = op(sdata[tid],sdata[tid + 8] );__threadfence_block();
        sdata[tid]= tmp;   __threadfence_block(); }
    if (blockSize >= 8)  { tmp = op(sdata[tid],sdata[tid + 4] ); __threadfence_block();
        sdata[tid]= tmp;  __threadfence_block(); }
    if (blockSize >= 4)  { tmp = op(sdata[tid],sdata[tid + 2] ); __threadfence_block();
        sdata[tid]= tmp;   __threadfence_block(); }
    if (blockSize >= 2)  { tmp = op(sdata[tid],sdata[tid + 1] ); __threadfence_block();
        sdata[tid]= tmp;  __threadfence_block();  }
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace hip

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} //end namespace Foam

// ************************************************************************* //

#endif

// ************************************************************************* //

