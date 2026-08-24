/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2018-2025 OpenCFD Ltd.
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "bitSet.H"
#include "IOstreams.H"
#include "UPstream.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(bitSet, 0);

    // TBD: add IO support of compound type?
    // defineNamedCompoundTypeName(bitSet, List<1>);
    // addNamedCompoundToRunTimeSelectionTable(bitSet, bitSet, List<1>);
}

// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

Foam::bitSet& Foam::bitSet::minusEq(const bitSet& other)
{
    if (&other == this)
    {
        // Self '-=' : clears all bits
        if (debug & 2)
        {
            InfoInFunction
                << "Perform -= on self: clears all bits" << nl;
        }

        reset();
        return *this;
    }
    else if (none() || other.none())
    {
        // no-op: nothing can change
        return *this;
    }

    // The operation (on overlapping blocks)
    {
        const label nblocks = num_blocks(std::min(size(), other.size()));
        const auto& rhs = other.blocks_;

        for (label blocki = 0; blocki < nblocks; ++blocki)
        {
            blocks_[blocki] &= ~rhs[blocki];
        }
    }

    return *this;
}


Foam::bitSet& Foam::bitSet::andEq(const bitSet& other)
{
    if (FOAM_UNLIKELY(&other == this))
    {
        // Self '&=' : no-op

        if (debug & 2)
        {
            InfoInFunction
                << "Perform &= on self: ignore" << nl;
        }

        return *this;
    }
    else if (none())
    {
        // no-op: nothing is set - no intersection possible
        return *this;
    }
    else if (other.none())
    {
        // no-op: other has nothing set - no intersection possible
        reset();
        return *this;
    }


    const label origSize(size());
    const label otherSize(other.size());

    if (origSize > otherSize)
    {
        // Clear bits (and blocks) that do not overlap at all
        resize(otherSize);
        resize(origSize);
    }

    // The operation (on overlapping blocks)
    {
        const label nblocks = num_blocks(std::min(origSize, otherSize));
        const auto& rhs = other.blocks_;

        for (label blocki = 0; blocki < nblocks; ++blocki)
        {
            blocks_[blocki] &= rhs[blocki];
        }
    }

    return *this;
}


Foam::bitSet& Foam::bitSet::orEq(const bitSet& other)
{
    if (&other == this)
    {
        // Self '|=' : no-op

        if (debug & 2)
        {
            InfoInFunction
                << "Perform |= on self: ignore" << nl;
        }

        return *this;
    }
    else if (other.none())
    {
        // no-op: nothing can change
        return *this;
    }


    // Largest new bit that could be introduced
    const label otherMax(other.find_last());

    if (otherMax >= size())
    {
        // Extend to accommodate bits from 'other'
        resize(otherMax+1);
    }

    // The operation (on overlapping blocks)
    {
        const label nblocks = num_blocks(std::min(size(), other.size()));
        const auto& rhs = other.blocks_;

        for (label blocki = 0; blocki < nblocks; ++blocki)
        {
            blocks_[blocki] |= rhs[blocki];
        }
    }

    return *this;
}


Foam::bitSet& Foam::bitSet::xorEq(const bitSet& other)
{
    if (&other == this)
    {
        // Self '^=' : clears all bits

        if (debug & 2)
        {
            InfoInFunction
                << "Perform ^= on self: clears all bits" << nl;
        }

        reset();
        return *this;
    }
    else if (other.none())
    {
        // no-op: nothing can change
        return *this;
    }


    // Largest new bit that could be introduced
    const label otherMax(other.find_last());

    if (otherMax >= size())
    {
        // Extend to accommodate bits from 'other'
        resize(otherMax+1);
    }

    // The operation (on overlapping blocks)
    {
        const label nblocks = num_blocks(std::min(size(), other.size()));
        const auto& rhs = other.blocks_;

        for (label blocki = 0; blocki < nblocks; ++blocki)
        {
            blocks_[blocki] ^= rhs[blocki];
        }
    }

    return *this;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::bitSet::bitSet(Istream& is)
:
    PackedList<1>()
{
    is  >> *this;
}


Foam::bitSet::bitSet(const bitSet& bitset, const labelUList& addr)
:
    bitSet(addr.size())
{
    const label len = addr.size();

    for (label i = 0; i < len; ++i)
    {
        set(i, bitset.get(addr[i]));
    }
}


Foam::bitSet::bitSet(const bitSet& bitset, const labelRange& range)
:
    bitSet(range.size())
{
    label pos = range.start();
    const label len = range.size();

    for (label i = 0; i < len; ++i)
    {
        set(i, bitset.get(pos));
        ++pos;
    }
}


Foam::bitSet::bitSet(const label n, const labelRange& range)
:
    bitSet(n)
{
    this->set(range);
}


Foam::bitSet::bitSet(const labelRange& range)
{
    this->set(range);
}


// * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * * //

void Foam::bitSet::assign(const UList<bool>& bools)
{
    fill(false);
    resize(bools.size());

    unsigned bitIdx = 0u;
    auto* packed = blocks_.data();

    // Set according to indices that are true
    for (const auto b : bools)
    {
        if (b)
        {
            *packed |= (1u << bitIdx);
        }

        if (++bitIdx >= PackedList<1>::elem_per_block)
        {
            bitIdx = 0u;
            ++packed;
        }
    }
}


bool Foam::bitSet::intersects(const bitSet& other) const
{
    if (size() && other.size())
    {
        const label nblocks = num_blocks(std::min(size(), other.size()));
        const auto& rhs = other.blocks_;

        for (label blocki = 0; blocki < nblocks; ++blocki)
        {
            if (bool(blocks_[blocki] & rhs[blocki]))
            {
                return true;
            }
        }
    }

    return false;
}


void Foam::bitSet::set(const labelRange& range)
{
    labelRange slice(range);
    slice.adjust();  // No negative start, size adjusted accordingly

    // Range is invalid (zero-sized or entirely negative) - noop
    if (slice.empty())
    {
        return;
    }

    // Range finishes at or beyond the right side.
    // - zero fill any gaps that we might create.
    // - flood-fill the reset, which now corresponds to the full range.
    //
    // NB: use labelRange end_value() for the exclusive end-value, which
    // corresponds to our new set size.
    if (slice.end_value() >= size())
    {
        reserve(slice.end_value());
        resize(slice.begin_value(), false);
        resize(slice.end_value(), true);
        return;
    }

    // The more difficult case - everything in between.
    // 1. sequence may begin/end in the same block
    // 2. Cover more than one block
    //    a. with partial coverage in the first block
    //    b. with partial coverage in the end block

    // The begin block/offset
    unsigned int bblock = slice.begin_value() / elem_per_block;
    unsigned int bmask  = slice.begin_value() % elem_per_block;

    // The end block/offset
    unsigned int eblock = slice.end_value() / elem_per_block;
    unsigned int emask  = slice.end_value() % elem_per_block;

    // Transform offsets to lower bit masks
    if (bmask) bmask = mask_lower(bmask);
    if (emask) emask = mask_lower(emask);

    if (bblock == eblock)
    {
        // Same block - flll between the begin/end bits.
        // Example:
        // bmask = 0000000000001111  (lower bits)
        // emask = 0000111111111111  (lower bits)
        // -> set  0000111111110000  (xor)

        blocks_[bblock] |= (emask^bmask);
    }
    else
    {
        if (bmask)
        {
            // The first (partial) block
            // - set everything above the bmask.
            blocks_[bblock] |= (~bmask);
            ++bblock;
        }

        // Fill these blocks
        for (unsigned blocki = bblock; blocki < eblock; ++blocki)
        {
            blocks_[blocki] = (~0u);
        }

        if (emask)
        {
            // The last (partial) block.
            // - set everything below emask.
            blocks_[eblock] |= (emask);
        }
    }
}


void Foam::bitSet::unset(const labelRange& range)
{
    // Require intersection with the current bitset
    const labelRange slice = range.subset0(size());

    // Range does not intersect (invalid, empty, bitset is empty)
    if (slice.empty())
    {
        return;
    }

    // Range finishes at or beyond the right side.
    //
    // NB: use labelRange end_value() for the exclusive end-value, which
    // corresponds to our new set size.
    if (slice.end_value() >= size())
    {
        // The original size
        const label orig = size();

        resize(slice.begin_value(), false);
        resize(orig, false);
        return;
    }


    // The more difficult case - everything in between.
    // 1. sequence may begin/end in the same block
    // 2. Cover more than one block
    //    a. with partial coverage in the first block
    //    b. with partial coverage in the end block

    // The begin block/offset
    unsigned int bblock = slice.begin_value() / elem_per_block;
    unsigned int bmask  = slice.begin_value() % elem_per_block;

    // The end block/offset
    unsigned int eblock = slice.end_value() / elem_per_block;
    unsigned int emask  = slice.end_value() % elem_per_block;

    // Transform offsets to lower bit masks
    if (bmask) bmask = mask_lower(bmask);
    if (emask) emask = mask_lower(emask);

    if (bblock == eblock)
    {
        // Same block - flll between the begin/end bits.
        // Example:
        // bmask = 0000000000001111  (lower bits)
        // emask = 0000111111111111  (lower bits)
        // -> set  0000111111110000  (xor)
        // -> ~    1111000000001111

        blocks_[bblock] &= (~(emask^bmask));
    }
    else
    {
        if (bmask)
        {
            // The first (partial) block
            // - only retain things below bmask.
            blocks_[bblock] &= (bmask);
            ++bblock;
        }

        // Clear these blocks
        for (unsigned blocki = bblock; blocki < eblock; ++blocki)
        {
            blocks_[blocki] = (0u);
        }

        if (emask)
        {
            // The last (partial) block.
            // - only retain things above bmask.
            blocks_[eblock] &= (~emask);
        }
    }
}


Foam::labelList Foam::bitSet::toc() const
{
    // Number of used (set) entries
    const label total = any() ? count() : 0;

    if (!total)
    {
        return labelList();
    }

    labelList output(total);
    label nItem = 0;

    // Process block-wise, detecting any '1' bits

    const label nblocks = num_blocks(size());
    for (label blocki = 0; blocki < nblocks; ++blocki)
    {
        unsigned int blockval = blocks_[blocki];

        if (blockval)
        {
            for (label pos = (blocki * elem_per_block); blockval; ++pos)
            {
                if (blockval & 1u)
                {
                    output[nItem] = pos;
                    ++nItem;
                }
                blockval >>= 1u;
            }
            if (nItem == total) break;  // Terminate early
        }
    }

    return output;
}


Foam::List<bool> Foam::bitSet::values() const
{
    List<bool> output(size(), false);

    // Process block-wise, detecting any '1' bits

    const label nblocks = num_blocks(size());
    for (label blocki = 0; blocki < nblocks; ++blocki)
    {
        label pos = (blocki * elem_per_block);

        for
        (
            unsigned int blockval = blocks_[blocki];
            blockval;
            blockval >>= 1u
        )
        {
            if (blockval & 1u)
            {
                output[pos] = true;
            }
            ++pos;
        }
    }

    return output;
}


// * * * * * * * * * * * * * *  Parallel Functions * * * * * * * * * * * * * //

namespace
{

// Special purpose broadcast for bitSet which is more efficient than
// either a regular broadcast (with serialization) or the usual
// broadcast for lists.
//
// The initial broadcast sends both count (bits=on) and the length.
// The receive clears out its bits and resizes (ie, all zeros and the proper
// length).
// This allows the final broadcast to be skipped if either
// the length or the content is zero.
//
// With syncSizes=false it assumes that the lengths are identical on all ranks
// and doesn't do the initial broadcast. In this case it can only decide
// about the final broadcast based on the length information allow.

void broadcast_bitSet
(
    Foam::bitSet& bitset,
    int communicator,
    int root,
    bool syncSizes
)
{
    using namespace Foam;

    if (!UPstream::is_parallel(communicator))  // Probably already checked...
    {
        return;
    }

    // Broadcast data content?
    bool bcastContent(true);

    if (syncSizes)
    {
        int64_t count_size[2] = { 0, 0 };

        if (root == UPstream::myProcNo(communicator))
        {
            // Sender: knows the count/size
            count_size[0] = static_cast<int64_t>(bitset.count());
            count_size[1] = static_cast<int64_t>(bitset.size());

            UPstream::broadcast(count_size, 2, communicator, root);
        }
        else
        {
            // Receiver: gets the count/size and makes a clean bitset
            UPstream::broadcast(count_size, 2, communicator, root);

            bitset.clear();  // Clear old contents
            bitset.resize(count_size[1]);
        }

        if (count_size[0] == 0)
        {
            // All content is zero, don't need to broadcast it
            bcastContent = false;
        }
    }

    if (bcastContent && !bitset.empty())
    {
        // Only broadcast with non-empty content
        UPstream::broadcast
        (
            bitset.data(),
            bitset.num_blocks(),
            communicator,
            root
        );
    }
}

} // End anonymous namespace


void Foam::bitSet::broadcast
(
    std::pair<int,int> communicator_root,
    bool syncSizes
)
{
    int comm = communicator_root.first;
    int root = communicator_root.second;

    if (UPstream::is_parallel(comm))
    {
        broadcast_bitSet(*this, comm, root, syncSizes);
    }
}


void Foam::bitSet::broadcast(int communicator, bool syncSizes)
{
    if (communicator < 0)
    {
        communicator = UPstream::worldComm;
    }

    if (UPstream::is_parallel(communicator))
    {
        broadcast_bitSet(*this, communicator, UPstream::masterNo(), syncSizes);
    }
}


void Foam::bitSet::reduceAnd(int communicator, bool syncSizes)
{
    if (communicator < 0)
    {
        communicator = UPstream::worldComm;
    }

    if (!UPstream::is_parallel(communicator))
    {
        return;
    }

    const label origSize(size());

    if (syncSizes)
    {
        // Operation is an intersection
        // - common size may be smaller than the original size
        int64_t commonSize(size());

        UPstream::mpiAllReduce<UPstream::opCodes::op_min>
        (
            &commonSize,
            1,
            communicator
        );
        resize(commonSize);
    }

    if (!empty())
    {
        UPstream::mpiAllReduce<UPstream::opCodes::op_bit_and>
        (
            this->data(),
            this->num_blocks(),
            communicator
        );

        clear_trailing_bits();  // safety
    }

    // Undo side effects from the reduction
    if (syncSizes)
    {
        resize(origSize);
    }
}


void Foam::bitSet::reduceOr(int communicator, bool syncSizes)
{
    if (communicator < 0)
    {
        communicator = UPstream::worldComm;
    }

    if (!UPstream::is_parallel(communicator))
    {
        return;
    }

    // const label origSize(size());

    if (syncSizes)
    {
        // Operation can increase the addressed size

        // Extend size based on the addressed length.
        // This is greedy, but produces consistent sizing
        int64_t commonSize(size());

        // Alternative: Extend size based on the bits used.
        // - tighter, but inconsistent sizes result
        // // label commonSize(find_last()+1);

        UPstream::mpiAllReduce<UPstream::opCodes::op_max>
        (
            &commonSize,
            1,
            communicator
        );

        extend(commonSize);
    }

    if (!empty())
    {
        UPstream::mpiAllReduce<UPstream::opCodes::op_bit_or>
        (
            this->data(),
            this->num_blocks(),
            communicator
        );

        clear_trailing_bits();  // safety
    }
}


Foam::bitSet Foam::bitSet::gatherValues(bool localValue, int communicator)
{
    if (communicator < 0)
    {
        communicator = UPstream::worldComm;
    }

    bitSet allValues;

    if (!UPstream::is_parallel(communicator))
    {
        // non-parallel: return own value
        // TBD: only when UPstream::is_rank(communicator) as well?
        allValues.resize(1);
        allValues.set(0, localValue);
    }
    else
    {
        List<bool> bools;
        if (UPstream::master(communicator))
        {
            bools.resize(UPstream::nProcs(communicator), false);
        }

        UPstream::mpiGather
        (
            &localValue,    // Send
            bools.data(),   // Recv
            1,              // Num send/recv data per rank
            communicator
        );

        // Transcribe to bitSet (on master)
        allValues.assign(bools);
    }

    return allValues;
}


// Note that for allGather()
// - MPI_Gather of individual bool values and broadcast the packed result
// - this avoids bit_or on 32bit values everywhere, since we know a priori
//   that each rank only contributes 1bit of info

Foam::bitSet Foam::bitSet::allGather(bool localValue, int communicator)
{
    if (communicator < 0)
    {
        communicator = UPstream::worldComm;
    }

    bitSet allValues(bitSet::gatherValues(localValue, communicator));

    if (UPstream::is_parallel(communicator))
    {
        // Identical size on all ranks
        allValues.resize(UPstream::nProcs(communicator));

        // Sizes are consistent - broadcast without resizing
        allValues.broadcast(communicator, false);
    }

    return allValues;
}


// ************************************************************************* //
