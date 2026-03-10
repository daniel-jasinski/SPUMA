/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2022 OpenCFD Ltd.
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

Description
    Macro definitions for declaring ClassName(), NamespaceName(), etc.

\*---------------------------------------------------------------------------*/

#ifndef Foam_className_H
#define Foam_className_H

#include "defineDebugSwitch.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// Declarations (without debug information)

//- Add typeName information from argument \a TypeNameString to a class.
//  Without debug information
#define ClassNameNoDebug(TypeNameString)                                       \
    static const char* baseTypeName() { return TypeNameString; }              \
    static const char* typeName_() { return TypeNameString; }                 \
    FOAM_TYPENAME_EXPORT static const ::Foam::word typeName;                  \
    FOAM_HOST_ONLY                                                            \
    static const ::Foam::word& staticTypeName()                               \
    {                                                                         \
        static const ::Foam::word tn(baseTypeName());                         \
        return tn;                                                            \
    }

//- Add typeName information from argument \a TypeNameString to a namespace.
//  Without debug information.
#define NamespaceNameNoDebug(TypeNameString)                                   \
    inline const char* baseTypeName() { return TypeNameString; }              \
    extern const ::Foam::word typeName;                                       \
    FOAM_HOST_ONLY                                                            \
    inline const ::Foam::word& staticTypeName()                               \
    {                                                                         \
        static const ::Foam::word tn(baseTypeName());                         \
        return tn;                                                            \
    }

//- Add typeName information from argument \a TemplateNameString to a
//  template class.  Without debug information.
#define TemplateNameNoDebug(TemplateNameString)                                \
class TemplateNameString##Name                                                 \
{                                                                              \
public:                                                                        \
    TemplateNameString##Name() noexcept = default;                             \
    ClassNameNoDebug(#TemplateNameString);                                     \
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// Declarations (with debug information)

//- Add typeName information from argument \a TypeNameString to a class.
//  Also declares debug information.
#define ClassName(TypeNameString)                                              \
    ClassNameNoDebug(TypeNameString);                                          \
    FOAM_TYPENAME_EXPORT static int debug;                                    \
    static int debugLevel()

//- Add typeName information from argument \a TypeNameString to a namespace.
//  Also declares debug information.
#define NamespaceName(TypeNameString)                                          \
    NamespaceNameNoDebug(TypeNameString);                                      \
    extern int debug;                                                         \
    int debugLevel()

//- Add typeName information from argument \a TypeNameString to a
//  template class.  Also declares debug information.
#define TemplateName(TemplateNameString)                                       \
class TemplateNameString##Name                                                 \
{                                                                              \
public:                                                                        \
    TemplateNameString##Name() noexcept = default;                             \
    ClassName(#TemplateNameString);                                            \
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// Definitions (without debug information)

//- Define the typeName, with alternative lookup as \a Name
#define defineTypeNameWithName(Type, Name)                                     \
    const ::Foam::word Type::typeName(Name)

//- Define the typeName
#define defineTypeName(Type)                                                   \
    defineTypeNameWithName(Type, Type::baseTypeName())

//- Define the typeName as \a Name for template classes
#define defineTemplateTypeNameWithName(Type, Name)                             \
    template<>                                                                 \
    defineTypeNameWithName(Type, Name)

//- Define the typeName for template classes, useful with typedefs
#define defineTemplateTypeName(Type)                                           \
    defineTemplateTypeNameWithName(Type, #Type)

//- Define the typeName directly for template classes
#define defineNamedTemplateTypeName(Type)                                      \
    defineTemplateTypeNameWithName(Type, Type::baseTypeName())


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// staticTypeName definitions

//- Define the staticTypeName function for non-template classes
//  (only needed when an override of the inline default is required)
//  Variadic to support types with commas (e.g. CompactIOList<face, label>)
#define defineStaticTypeNameFunction(...)                                      \
    const ::Foam::word& __VA_ARGS__::staticTypeName()                          \
    { return __VA_ARGS__::typeName; }

//- Define the staticTypeName function for template specializations
//  Variadic to support types with commas (e.g. CompactIOList<face, label>)
#define defineTemplateStaticTypeNameFunction(...)                              \
    template<>                                                                 \
    const ::Foam::word& __VA_ARGS__::staticTypeName()                          \
    { return __VA_ARGS__::typeName; }

//- Forward-declare the staticTypeName specialization (for use in headers)
//  Variadic to support types with commas (e.g. CompactIOList<face, label>)
#define declareStaticTypeName(...)                                             \
    template<> const ::Foam::word& __VA_ARGS__::staticTypeName()


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// Definitions (with debug information)

//- Define the typeName and debug information
#define defineTypeNameAndDebug(Type, DebugSwitch)                              \
    defineTypeName(Type);                                                      \
    defineDebugSwitch(Type, DebugSwitch);                                      \
    defineDebugLevelFunction(Type)

//- Define the typeName and debug information, lookup as \a Name
#define defineTemplateTypeNameAndDebugWithName(Type, Name, DebugSwitch)        \
    defineTemplateTypeNameWithName(Type, Name);                                \
    defineTemplateDebugSwitchWithName(Type, Name, DebugSwitch);                \
    defineTemplateDebugLevelFunction(Type);                                    \
    defineTemplateStaticTypeNameFunction(Type)

//- Define the typeName and debug information for templates, useful
//  with typedefs
#define defineTemplateTypeNameAndDebug(Type, DebugSwitch)                      \
    defineTemplateTypeNameAndDebugWithName(Type, #Type, DebugSwitch)

//- Define the typeName and debug information for templates
#define defineNamedTemplateTypeNameAndDebug(Type, DebugSwitch)                 \
    defineNamedTemplateTypeName(Type);                                         \
    defineNamedTemplateDebugSwitch(Type, DebugSwitch);                         \
    defineTemplateDebugLevelFunction(Type);                                    \
    defineTemplateStaticTypeNameFunction(Type)


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// Old names

// Same as defineTemplateTypeNameWithName
#define defineTemplate2TypeNameWithName(Type, Name)                            \
    defineTemplateTypeNameWithName(Type, Name)

// Same as defineTemplateTypeNameAndDebugWithName
#define defineTemplate2TypeNameAndDebugWithName(Type, Name, DebugSwitch)       \
    defineTemplateTypeNameWithName(Type, Name);                                \
    defineTemplateDebugSwitchWithName(Type, Name, DebugSwitch);                \
    defineTemplateDebugLevelFunction(Type);                                    \
    defineTemplateStaticTypeNameFunction(Type)

// Same as defineTemplateTypeNameAndDebug
#define defineTemplate2TypeNameAndDebug(Type, DebugSwitch)                     \
    defineTemplateTypeNameAndDebug(Type, DebugSwitch)


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
