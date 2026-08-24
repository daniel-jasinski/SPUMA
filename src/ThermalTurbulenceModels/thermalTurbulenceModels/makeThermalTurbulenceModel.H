/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2016 OpenFOAM Foundation
    Copyright (C) 2020 OpenCFD Ltd.
    Copyright (C) 2026 Cineca
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

//- Define thermal turbulence model typedefs, with EddyDiffusivity
#define defineThermalTurbulenceModelTypes(                                     \
    Alpha, Rho, baseModel, BaseModel, Thermo, Transport)                       \
                                                                               \
    namespace Foam                                                             \
    {                                                                          \
        typedef ThermalTurbulenceModel                                         \
        <                                                                      \
            Alpha,                                                             \
            Rho,                                                               \
            baseModel,                                                         \
            Thermo,                                                            \
	    Transport                                                          \
        > Thermo##Transport##baseModel;                                        \
                                                                               \
        typedef BaseModel<Thermo, Transport>                                   \
            Thermo##Transport##BaseModel;                                      \
                                                                               \
        typedef laminarThermalModel<Thermo##Transport##BaseModel>              \
            laminar##Thermo##Transport##BaseModel;                             \
                                                                               \
        typedef thermalRASModel<Thermo##Transport##BaseModel>                  \
            RAS##Thermo##Transport##BaseModel;                                 \
    }                                                                          \

// Legacy name
#define makeThermalTurbulenceModelTypes(a, b, c, d, e, f) \
    defineThermalTurbulenceModelTypes(a, b, c, d, e, f)


#define makeBaseThermalTurbulenceModel(Alpha, Rho, baseModel, BaseModel, Thermo, Transport)   \
    /* Turbulence typedefs */                                                                 \
    defineThermalTurbulenceModelTypes(Alpha, Rho, baseModel, BaseModel, Thermo, Transport)    \
                                                                                              \
    namespace Foam                                                                            \
    {                                                                                         \
        /* Turbulence selection table */                                                      \
        defineTemplateRunTimeSelectionTable                                                   \
        (                                                                                     \
            Thermo##Transport##baseModel,                                                     \
            dictionary                                                                        \
        );                                                                                    \
                                                                                              \
        /* Laminar models */                                                                  \
        defineNamedTemplateTypeNameAndDebug(laminar##Thermo##Transport##BaseModel, 0);        \
                                                                                              \
        defineTemplateRunTimeSelectionTable                                                   \
        (laminar##Thermo##Transport##BaseModel, dictionary);                                  \
                                                                                              \
        addToRunTimeSelectionTable                                                            \
        (                                                                                     \
            Thermo##Transport##baseModel,                                                     \
            laminar##Thermo##Transport##BaseModel,                                            \
            dictionary                                                                        \
        );                                                                                    \
                                                                                              \
	/* RAS models */                                                                      \
        defineNamedTemplateTypeNameAndDebug(RAS##Thermo##Transport##BaseModel, 0);            \
                                                                                              \
        defineTemplateRunTimeSelectionTable                                                   \
        (RAS##Thermo##Transport##BaseModel, dictionary);                                      \
                                                                                              \
        addToRunTimeSelectionTable                                                            \
        (                                                                                     \
            Thermo##Transport##baseModel,                                                     \
            RAS##Thermo##Transport##BaseModel,                                                \
            dictionary                                                                        \
        );                                                                                    \
    }


#define makeTemplatedThermalTurbulenceModel(BaseModel, SType, Type)            \
    defineNamedTemplateTypeNameAndDebug                                        \
        (Foam::SType##Models::Type<Foam::BaseModel>, 0);                       \
                                                                               \
    namespace Foam                                                             \
    {                                                                          \
        namespace SType##Models                                                \
        {                                                                      \
            typedef Type<BaseModel> Type##SType##BaseModel;                    \
                                                                               \
            addToRunTimeSelectionTable                                         \
            (                                                                  \
                SType##BaseModel,                                              \
                Type##SType##BaseModel,                                        \
                dictionary                                                     \
            );                                                                 \
        }                                                                      \
    }


#define makeThermalTurbulenceModel(BaseModel, SType, Type)                     \
    namespace Foam                                                             \
    {                                                                          \
        namespace SType##Models                                                \
        {                                                                      \
            defineTypeNameAndDebug(Type, 0);                                   \
                                                                               \
            addToRunTimeSelectionTable                                         \
            (                                                                  \
                SType##BaseModel,                                              \
                Type,                                                          \
                dictionary                                                     \
            );                                                                 \
        }                                                                      \
    }


// ************************************************************************* //
