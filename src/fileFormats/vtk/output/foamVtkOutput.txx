/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016-2025 OpenCFD Ltd.
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

#include "globalIndex.H"
#include "ListOps.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type>
inline void Foam::vtk::write
(
    vtk::formatter& fmt,
    const Type& val,
    const label n
)
{
    if constexpr
    (
        std::is_same_v<Vector<float>, Type>
     || std::is_same_v<Vector<double>, Type>
    )
    {
        // Vector is frequently used
        for (label i = 0; i < n; ++i)
        {
            fmt.write(val.x());
            fmt.write(val.y());
            fmt.write(val.z());
        }
    }
    else if constexpr
    (
        std::is_same_v<SymmTensor<float>, Type>
     || std::is_same_v<SymmTensor<double>, Type>
    )
    {
        // VTK order is (XX, YY, ZZ, XY, YZ, XZ)
        for (label i = 0; i < n; ++i)
        {
            fmt.write(val.xx());
            fmt.write(val.yy());
            fmt.write(val.zz());
            fmt.write(val.xy());
            fmt.write(val.yz());
            fmt.write(val.xz());
        }
    }
    else if constexpr (is_vectorspace_v<Type>)
    {
        constexpr direction nCmpt = pTraits<Type>::nComponents;

        for (label i = 0; i < n; ++i)
        {
            for (direction cmpt = 0; cmpt < nCmpt; ++cmpt)
            {
                fmt.write(component(val, cmpt));
            }
        }
    }
    else
    {
        // label, scalar etc.

        for (label i = 0; i < n; ++i)
        {
            fmt.write(val);
        }
    }
}


template<class Type>
void Foam::vtk::writeList
(
    vtk::formatter& fmt,
    const UList<Type>& values
)
{
    for (const Type& val : values)
    {
        vtk::write(fmt, val);
    }
}


template<class Type, unsigned N>
void Foam::vtk::writeList
(
    vtk::formatter& fmt,
    const FixedList<Type, N>& values
)
{
    for (const Type& val : values)
    {
        vtk::write(fmt, val);
    }
}


template<class Type>
void Foam::vtk::writeList
(
    vtk::formatter& fmt,
    const UList<Type>& values,
    const labelUList& addressing
)
{
    for (const label idx : addressing)
    {
        vtk::write(fmt, values[idx]);
    }
}


template<class Type>
void Foam::vtk::writeList
(
    vtk::formatter& fmt,
    const UList<Type>& values,
    const bitSet& selected
)
{
    for (const label idx : selected)
    {
        vtk::write(fmt, values[idx]);
    }
}


template<class Type>
void Foam::vtk::writeLists
(
    vtk::formatter& fmt,
    const UList<Type>& values,
    const UList<Type>& indirect,
    const labelUList& addressing
)
{
    vtk::writeList(fmt, values);
    vtk::writeList(fmt, indirect, addressing);
}


template<class Type>
void Foam::vtk::writeValueParallel
(
    vtk::formatter& fmt,
    const Type& val,
    const label count
)
{
    if constexpr (!is_contiguous_v<Type>)
    {
        // Non-contiguous data does not make sense
        FatalErrorInFunction
            << "Contiguous data only" << endl
            << Foam::exit(FatalError);
    }

    // Gather [count, value] tuples, including from master
    const List<label> counts(UPstream::listGatherValues(count));
    const List<Type> values(UPstream::listGatherValues(val));

    if (UPstream::master())
    {
        forAll(counts, i)
        {
            // Write [value, count] tuple
            vtk::write(fmt, values[i], counts[i]);
        }
    }
}


template<class Type>
void Foam::vtk::writeListParallel
(
    vtk::formatter& fmt,
    const UList<Type>& values
)
{
    if constexpr (!is_contiguous_v<Type>)
    {
        // Non-contiguous data does not make sense
        FatalErrorInFunction
            << "Contiguous data only" << endl
            << Foam::exit(FatalError);
    }

    // The receive sizes
    const labelList recvSizes(globalIndex::calcRecvSizes(values.size()));

    if (UPstream::master())
    {
        const label maxRecvSize = recvSizes[0];

        // Write master data
        vtk::writeList(fmt, values);

        // Receive and write
        DynamicList<Type> recvData(maxRecvSize);

        for (const int proci : UPstream::subProcs())
        {
            if (label procSize = recvSizes[proci]; procSize > 0)
            {
                recvData.resize_nocopy(procSize);
                UIPstream::read
                (
                    UPstream::commsTypes::scheduled,
                    proci,
                    recvData
                );
                vtk::writeList(fmt, recvData);
            }
        }
    }
    else
    {
        if (values.size())
        {
            UOPstream::write
            (
                UPstream::commsTypes::scheduled,
                UPstream::masterNo(),
                values
            );
        }
    }
}


template<class Type>
void Foam::vtk::writeListParallel
(
    vtk::formatter& fmt,
    const UList<Type>& values,
    const labelUList& addressing
)
{
    if constexpr (!is_contiguous_v<Type>)
    {
        // Non-contiguous data does not make sense
        FatalErrorInFunction
            << "Contiguous data only" << endl
            << Foam::exit(FatalError);
    }


    List<Type> sendData;
    if (!UPstream::master())
    {
        sendData = UIndirectList<Type>(values, addressing);
    }

    // The receive sizes
    const labelList recvSizes(globalIndex::calcRecvSizes(sendData.size()));

    if (UPstream::master())
    {
        const label maxRecvSize = recvSizes[0];

        // Write master data
        vtk::writeList(fmt, values, addressing);

        // Receive and write
        DynamicList<Type> recvData(maxRecvSize);

        for (const int proci : UPstream::subProcs())
        {
            if (label procSize = recvSizes[proci]; procSize > 0)
            {
                recvData.resize_nocopy(procSize);
                UIPstream::read
                (
                    UPstream::commsTypes::scheduled,
                    proci,
                    recvData
                );
                vtk::writeList(fmt, recvData);
            }
        }
    }
    else
    {
        if (sendData.size())
        {
            UOPstream::write
            (
                UPstream::commsTypes::scheduled,
                UPstream::masterNo(),
                sendData
            );
        }
    }
}


template<class Type>
void Foam::vtk::writeListParallel
(
    vtk::formatter& fmt,
    const UList<Type>& values,
    const bitSet& selected
)
{
    if constexpr (!is_contiguous_v<Type>)
    {
        // Non-contiguous data does not make sense
        FatalErrorInFunction
            << "Contiguous data only" << endl
            << Foam::exit(FatalError);
    }


    List<Type> sendData;
    if (!UPstream::master())
    {
        sendData = subset(selected, values);
    }

    // The receive sizes
    const labelList recvSizes(globalIndex::calcRecvSizes(sendData.size()));

    if (UPstream::master())
    {
        const label maxRecvSize = recvSizes[0];

        // Write master data
        vtk::writeList(fmt, values, selected);

        // Receive and write
        DynamicList<Type> recvData(maxRecvSize);

        for (const int proci : UPstream::subProcs())
        {
            if (label procSize = recvSizes[proci]; procSize > 0)
            {
                recvData.resize_nocopy(procSize);

                UIPstream::read
                (
                    UPstream::commsTypes::scheduled,
                    proci,
                    recvData
                );
                vtk::writeList(fmt, recvData);
            }
        }
    }
    else
    {
        if (sendData.size())
        {
            UOPstream::write
            (
                UPstream::commsTypes::scheduled,
                UPstream::masterNo(),
                sendData
            );
        }
    }
}


template<class Type>
void Foam::vtk::writeListsParallel
(
    vtk::formatter& fmt,
    const UList<Type>& values1,
    const UList<Type>& values2
)
{
    if constexpr (!is_contiguous_v<Type>)
    {
        // Non-contiguous data does not make sense
        FatalErrorInFunction
            << "Contiguous data only" << endl
            << Foam::exit(FatalError);
    }


    // The receive sizes
    const labelList recvSizes1(globalIndex::calcRecvSizes(values1.size()));
    const labelList recvSizes2(globalIndex::calcRecvSizes(values2.size()));

    if (UPstream::master())
    {
        const label maxRecvSize = std::max(recvSizes1[0], recvSizes2[0]);

        // Write master data
        vtk::writeList(fmt, values1);
        vtk::writeList(fmt, values2);

        // Receive and write
        DynamicList<Type> recvData(maxRecvSize);

        for (const int proci : UPstream::subProcs())
        {
            // values1
            if (label procSize = recvSizes1[proci]; procSize > 0)
            {
                recvData.resize_nocopy(procSize);
                UIPstream::read
                (
                    UPstream::commsTypes::scheduled,
                    proci,
                    recvData
                );
                vtk::writeList(fmt, recvData);
            }

            // values2
            if (label procSize = recvSizes2[proci]; procSize > 0)
            {
                recvData.resize_nocopy(procSize);
                UIPstream::read
                (
                    UPstream::commsTypes::scheduled,
                    proci,
                    recvData
                );
                vtk::writeList(fmt, recvData);
            }
        }
    }
    else
    {
        if (values1.size())
        {
            UOPstream::write
            (
                UPstream::commsTypes::scheduled,
                UPstream::masterNo(),
                values1
            );
        }

        if (values2.size())
        {
            UOPstream::write
            (
                UPstream::commsTypes::scheduled,
                UPstream::masterNo(),
                values2
            );
        }
    }
}


template<class Type>
void Foam::vtk::writeListsParallel
(
    vtk::formatter& fmt,
    const UList<Type>& values1,
    const UList<Type>& values2,
    const labelUList& addressing
)
{
    if constexpr (!is_contiguous_v<Type>)
    {
        // Non-contiguous data does not make sense
        FatalErrorInFunction
            << "Contiguous data only" << endl
            << Foam::exit(FatalError);
    }


    List<Type> sendData2;
    if (!UPstream::master())
    {
        sendData2 = UIndirectList<Type>(values2, addressing);
    }

    // The receive sizes
    const labelList recvSizes1(globalIndex::calcRecvSizes(values1.size()));
    const labelList recvSizes2(globalIndex::calcRecvSizes(sendData2.size()));

    if (UPstream::master())
    {
        const label maxRecvSize = std::max(recvSizes1[0], recvSizes2[0]);

        // Write master data
        vtk::writeList(fmt, values1);
        vtk::writeList(fmt, values2, addressing);

        // Receive and write
        DynamicList<Type> recvData(maxRecvSize);

        for (const int proci : UPstream::subProcs())
        {
            // values1
            if (label procSize = recvSizes1[proci]; procSize > 0)
            {
                recvData.resize_nocopy(procSize);
                UIPstream::read
                (
                    UPstream::commsTypes::scheduled,
                    proci,
                    recvData
                );
                vtk::writeList(fmt, recvData);
            }

            // values2
            if (label procSize = recvSizes2[proci]; procSize > 0)
            {
                recvData.resize_nocopy(procSize);
                UIPstream::read
                (
                    UPstream::commsTypes::scheduled,
                    proci,
                    recvData
                );
                vtk::writeList(fmt, recvData);
            }
        }
    }
    else
    {
        if (values1.size())
        {
            UOPstream::write
            (
                UPstream::commsTypes::scheduled,
                UPstream::masterNo(),
                values1
            );
        }

        if (sendData2.size())
        {
            UOPstream::write
            (
                UPstream::commsTypes::scheduled,
                UPstream::masterNo(),
                sendData2
            );
        }
    }
}


// ************************************************************************* //
