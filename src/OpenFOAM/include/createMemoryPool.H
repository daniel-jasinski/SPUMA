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
    Create a memory pool from arglist options

\*---------------------------------------------------------------------------*/

std::fprintf(stderr, "TRACE: createMemoryPool step 1 - before getOrDefault\n");
std::fflush(stderr);
const word pool = args.getOrDefault<word>("pool", "dummyMemoryPool");
const scalar poolSize = args.getOrDefault<scalar>("poolSize", 0);
std::fprintf(stderr, "TRACE: createMemoryPool step 2 - pool='%s' poolSize=%g\n", pool.c_str(), poolSize);
std::fflush(stderr);

std::fprintf(stderr, "TRACE: createMemoryPool step 3 - before Info\n");
std::fflush(stderr);
Info << "Create " << pool <<"\n " << endl;
std::fprintf(stderr, "TRACE: createMemoryPool step 4 - after Info\n");
std::fflush(stderr);
MemoryPool* memPool = MemoryPool::New(pool, poolSize*1024*1024*1024ul);
std::fprintf(stderr, "TRACE: createMemoryPool step 5 - after MemoryPool::New\n");
std::fflush(stderr);

// ************************************************************************* //
