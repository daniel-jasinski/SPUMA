/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2015-2017 OpenFOAM Foundation
    Copyright (C) 2016-2023 OpenCFD Ltd.
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

#include "IOobject.H"
#include "IOstreams.H"
#include "fileOperation.H"  // legacy include
#include "Istream.H"  // legacy include
#include "Pstream.H"  // legacy include

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
bool Foam::IOobject::typeHeaderOk
(
    const bool checkType,
    const bool search,
    const bool verbose
)
{
    // Use typeName_() instead of typeName to avoid cross-DLL data access
    // issues on Windows. typeName is a static word (data), which gets a
    // local uninitialized copy via FOAM_TYPENAME_EXPORT dllexport.
    // typeName_() returns const char* via a function call (thunk works).
    return readAndCheckHeader
    (
        is_globalIOobject<Type>::value,
        Foam::word(Type::typeName_()),
        checkType,
        search,
        verbose
    );
}


template<class Type>
Foam::fileName Foam::IOobject::typeFilePath(const bool search) const
{
    // Use typeName_() to avoid cross-DLL data access issues on Windows
    const Foam::word tName(Type::typeName_());
    return
    (
        is_globalIOobject<Type>::value
      ? this->globalFilePath(tName, search)
      : this->localFilePath(tName, search)
    );
}


template<class Type>
void Foam::IOobject::warnNoRereading() const
{
    if (readOpt() == IOobjectOption::READ_MODIFIED)
    {
        WarningInFunction
            << Type::typeName_() << ' ' << name()
            << " constructed with READ_MODIFIED but "
            << Type::typeName_() << " does not support automatic rereading."
            << endl;
    }
}


// ************************************************************************* //
