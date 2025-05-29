/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2023 OpenCFD Ltd.
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

#include "symmTransformField.H"
#include "FieldM.H"

// * * * * * * * * * * * * * * * Global Functions  * * * * * * * * * * * * * //

template<class Type>
void Foam::transform
(
    Field<Type>& result,
    const symmTensor& rot,
    const Field<Type>& fld
)
{
    if (result.usePool() && fld.usePool())
    {
        checkFields(result, fld, "f1 = tranform(s, f2)");
        foamExecutor exec;
        auto res_p = result.begin();
        const auto fld_p = fld.cbegin();
        auto Lambda = [=](label i){
            res_p[i] = transform(rot,fld_p[i]);
        };
        exec.parallelFor(Lambda,result.size());
    }
    else
    {
        TFOR_ALL_F_OP_FUNC_S_F
        (
            Type, result, =, transform, symmTensor, rot, Type, fld
        );
    }
}


template<class Type>
void Foam::transform
(
    Field<Type>& result,
    const symmTensorField& rot,
    const Field<Type>& fld
)
{
    if (rot.size() == 1)
    {
        if (result.usePool() && rot.usePool() && fld.usePool())
        {
            checkFields(result, rot, fld, "f1 = transform(f2, f3)");
            foamExecutor exec;
            auto res_p = result.begin();
            const auto rot_p = rot.cbegin();
            const auto fld_p = fld.cbegin();
            auto Lambda = [=](label i){
                res_p[i] = transform(rot_p[0],fld_p[i]); // direct acces to avoid page fault
            };
            exec.parallelFor(Lambda,result.size());
        }
        else
        {
            return transform(result, rot.front(), fld);
        }
    }

    if (result.usePool() && rot.usePool() && fld.usePool())
    {
        checkFields(result, rot, fld, "f1 = transform(f2, f3)");
        foamExecutor exec;
        auto res_p = result.begin();
        const auto rot_p = rot.cbegin();
        const auto fld_p = fld.cbegin();
        auto Lambda = [=](label i){
            res_p[i] = transform(rot_p[i],fld_p[i]);
        };
        exec.parallelFor(Lambda,result.size());
    }
    else{
        TFOR_ALL_F_OP_FUNC_F_F
        (
            Type, result, =, transform, symmTensor, rot, Type, fld
        );
    }
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::transform
(
    const symmTensorField& rot,
    const Field<Type>& fld
)
{
    auto tresult = tmp<Field<Type>>::New(fld.size());
    transform(tresult.ref(), rot, fld);
    return tresult;
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::transform
(
    const symmTensorField& rot,
    const tmp<Field<Type>>& tfld
)
{
    tmp<Field<Type>> tresult = New(tfld);
    transform(tresult.ref(), rot, tfld());
    tfld.clear();
    return tresult;
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::transform
(
    const tmp<symmTensorField>& trot,
    const Field<Type>& fld
)
{
    auto tresult = tmp<Field<Type>>::New(fld.size());
    transform(tresult.ref(), trot(), fld);
    trot.clear();
    return tresult;
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::transform
(
    const tmp<symmTensorField>& trot,
    const tmp<Field<Type>>& tfld
)
{
    tmp<Field<Type>> tresult = New(tfld);
    transform(tresult.ref(), trot(), tfld());
    trot.clear();
    tfld.clear();
    return tresult;
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::transform
(
    const symmTensor& rot,
    const Field<Type>& fld
)
{
    auto tresult = tmp<Field<Type>>::New(fld.size());
    transform(tresult.ref(), rot, fld);
    return tresult;
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::transform
(
    const symmTensor& rot,
    const tmp<Field<Type>>& tfld
)
{
    tmp<Field<Type>> tresult = New(tfld);
    transform(tresult.ref(), rot, tfld());
    tfld.clear();
    return tresult;
}


// ************************************************************************* //
