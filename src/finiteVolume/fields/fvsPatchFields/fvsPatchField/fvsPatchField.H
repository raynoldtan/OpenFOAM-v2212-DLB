/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2019-2022 OpenCFD Ltd.
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

Class
    Foam::fvsPatchField

Description
    An abstract base class with a fat-interface to all derived classes
    covering all possible ways in which they might be used.

    The first level of derivation is to basic patchFields which cover
    zero-gradient, fixed-gradient, fixed-value and mixed conditions.

    The next level of derivation covers all the specialised typed with
    specific evaluation procedures, particularly with respect to specific
    fields.

SourceFiles
    fvsPatchField.C
    fvsPatchFieldNew.C

\*---------------------------------------------------------------------------*/

#ifndef Foam_fvsPatchField_H
#define Foam_fvsPatchField_H

#include "fvPatch.H"
#include "DimensionedField.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
class objectRegistry;
class dictionary;
class fvPatchFieldMapper;
class surfaceMesh;

template<class Type> class fvsPatchField;
template<class Type> class calculatedFvsPatchField;

template<class Type>
Ostream& operator<<(Ostream&, const fvsPatchField<Type>&);


/*---------------------------------------------------------------------------*\
                      Class fvsPatchFieldBase Declaration
\*---------------------------------------------------------------------------*/

//- Template invariant parts for fvsPatchField
class fvsPatchFieldBase
{
    // Private Data

        //- Reference to patch
        const fvPatch& patch_;

protected:

    // Protected Member Functions

        //- Read dictionary entries.
        //  Useful when initially constructed without a dictionary
        virtual void readDict(const dictionary& dict);


public:

    //- Debug switch to disallow the use of generic fvsPatchField
    static int disallowGenericPatchField;

    //- Runtime type information
    TypeName("fvsPatchField");


    // Constructors

        //- Construct from patch
        explicit fvsPatchFieldBase(const fvPatch& p);

        //- Construct from patch and dictionary (unused)
        fvsPatchFieldBase(const fvPatch& p, const dictionary& dict);

        //- Copy construct with new patch
        fvsPatchFieldBase(const fvsPatchFieldBase& rhs, const fvPatch& p);

        //- Copy construct
        fvsPatchFieldBase(const fvsPatchFieldBase& rhs);


    //- Destructor
    virtual ~fvsPatchFieldBase() = default;


    // Member Functions

    // Access

        //- The associated objectRegistry
        const objectRegistry& db() const;

        //- Return the patch
        const fvPatch& patch() const noexcept
        {
            return patch_;
        }


    // Check

        //- Check that patches are identical
        void checkPatch(const fvsPatchFieldBase& rhs) const;
};


/*---------------------------------------------------------------------------*\
                        Class fvsPatchField Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class fvsPatchField
:
    public fvsPatchFieldBase,
    public Field<Type>
{
    // Private Data

        //- Reference to internal field
        const DimensionedField<Type, surfaceMesh>& internalField_;


public:

    //- The internal field type associated with the patch field
    typedef DimensionedField<Type, surfaceMesh> Internal;

    //- The patch type for the patch field
    typedef fvPatch Patch;

    //- Type for a \em calculated patch
    typedef calculatedFvsPatchField<Type> Calculated;


    // Declare run-time constructor selection tables

        declareRunTimeSelectionTable
        (
            tmp,
            fvsPatchField,
            patch,
            (
                const fvPatch& p,
                const DimensionedField<Type, surfaceMesh>& iF
            ),
            (p, iF)
        );

        declareRunTimeSelectionTable
        (
            tmp,
            fvsPatchField,
            patchMapper,
            (
                const fvsPatchField<Type>& ptf,
                const fvPatch& p,
                const DimensionedField<Type, surfaceMesh>& iF,
                const fvPatchFieldMapper& m
            ),
            (dynamic_cast<const fvsPatchFieldType&>(ptf), p, iF, m)
        );

        declareRunTimeSelectionTable
        (
            tmp,
            fvsPatchField,
            dictionary,
            (
                const fvPatch& p,
                const DimensionedField<Type, surfaceMesh>& iF,
                const dictionary& dict
            ),
            (p, iF, dict)
        );


    // Constructors

        //- Construct from patch and internal field
        fvsPatchField
        (
            const fvPatch&,
            const DimensionedField<Type, surfaceMesh>&
        );

        //- Construct from patch and internal field and patch field
        fvsPatchField
        (
            const fvPatch&,
            const DimensionedField<Type, surfaceMesh>&,
            const Field<Type>&
        );

        //- Construct from patch, internal field and dictionary
        fvsPatchField
        (
            const fvPatch&,
            const DimensionedField<Type, surfaceMesh>&,
            const dictionary&,
            const bool valueRequired=true
        );

        //- Construct by mapping the given fvsPatchField onto a new patch
        fvsPatchField
        (
            const fvsPatchField<Type>&,
            const fvPatch&,
            const DimensionedField<Type, surfaceMesh>&,
            const fvPatchFieldMapper&
        );

        //- Construct as copy
        fvsPatchField(const fvsPatchField<Type>&);

        //- Construct as copy setting internal field reference
        fvsPatchField
        (
            const fvsPatchField<Type>&,
            const DimensionedField<Type, surfaceMesh>&
        );

        //- Construct and return a clone
        virtual tmp<fvsPatchField<Type>> clone() const
        {
            return tmp<fvsPatchField<Type>>::New(*this);
        }

        //- Construct and return a clone setting internal field reference
        virtual tmp<fvsPatchField<Type>> clone
        (
            const DimensionedField<Type, surfaceMesh>& iF
        ) const
        {
            return tmp<fvsPatchField<Type>>::New(*this, iF);
        }


    // Selectors

        //- Return a pointer to a new patchField created on freestore given
        //  patch and internal field
        //  (does not set the patch field values)
        static tmp<fvsPatchField<Type>> New
        (
            const word& patchFieldType,
            const fvPatch&,
            const DimensionedField<Type, surfaceMesh>&
        );

        //- Return a pointer to a new patchField created on freestore given
        //  patch and internal field
        //  (does not set the patch field values)
        //  Allows override of constraint type
        static tmp<fvsPatchField<Type>> New
        (
            const word& patchFieldType,
            const word& actualPatchType,
            const fvPatch&,
            const DimensionedField<Type, surfaceMesh>&
        );

        //- Return a pointer to a new patchField created on freestore from
        //  a given fvsPatchField mapped onto a new patch
        static tmp<fvsPatchField<Type>> New
        (
            const fvsPatchField<Type>&,
            const fvPatch&,
            const DimensionedField<Type, surfaceMesh>&,
            const fvPatchFieldMapper&
        );

        //- Return a pointer to a new patchField created on freestore
        //  from dictionary
        static tmp<fvsPatchField<Type>> New
        (
            const fvPatch&,
            const DimensionedField<Type, surfaceMesh>&,
            const dictionary&
        );

        //- Return a pointer to a new calculatedFvsPatchField created on
        //  freestore without setting patchField values
        static tmp<fvsPatchField<Type>> NewCalculatedType
        (
            const fvPatch&
        );

        //- Return a pointer to a new calculatedFvsPatchField created on
        //  freestore without setting patchField values
        template<class Type2>
        static tmp<fvsPatchField<Type>> NewCalculatedType
        (
            const fvsPatchField<Type2>&
        );


    //- Destructor
    virtual ~fvsPatchField() = default;


    // Member Functions

        //- The type name for calculated patch fields
        static const word& calculatedType();


        // Attributes

            //- True if this patch field fixes a value.
            //  Needed to check if a level has to be specified while solving
            //  Poissons equations.
            virtual bool fixesValue() const
            {
                return false;
            }

            //- True if this patch field is coupled
            virtual bool coupled() const
            {
                return false;
            }


        // Access

            //- Return dimensioned internal field reference
            const DimensionedField<Type, surfaceMesh>& internalField()
            const noexcept
            {
                return internalField_;
            }

            //- Return internal field reference
            const Field<Type>& primitiveField() const noexcept
            {
                return internalField_;
            }


        // Mapping Functions

            //- Map (and resize as needed) from self given a mapping object
            virtual void autoMap
            (
                const fvPatchFieldMapper&
            );

            //- Reverse map the given fvsPatchField onto this fvsPatchField
            virtual void rmap
            (
                const fvsPatchField<Type>&,
                const labelList&
            );


        //- Write
        virtual void write(Ostream&) const;


        // Check

            //- Check fvsPatchField<Type> against given fvsPatchField<Type>
            void check(const fvsPatchField<Type>&) const;


    // Member Operators

        virtual void operator=(const UList<Type>&);

        virtual void operator=(const fvsPatchField<Type>&);
        virtual void operator+=(const fvsPatchField<Type>&);
        virtual void operator-=(const fvsPatchField<Type>&);
        virtual void operator*=(const fvsPatchField<scalar>&);
        virtual void operator/=(const fvsPatchField<scalar>&);

        virtual void operator+=(const Field<Type>&);
        virtual void operator-=(const Field<Type>&);

        virtual void operator*=(const Field<scalar>&);
        virtual void operator/=(const Field<scalar>&);

        virtual void operator=(const Type&);
        virtual void operator+=(const Type&);
        virtual void operator-=(const Type&);
        virtual void operator*=(const scalar);
        virtual void operator/=(const scalar);


        // Force an assignment irrespective of form of patch

        virtual void operator==(const fvsPatchField<Type>&);
        virtual void operator==(const Field<Type>&);
        virtual void operator==(const Type&);


    // Ostream Operator

        friend Ostream& operator<< <Type>(Ostream&, const fvsPatchField<Type>&);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "fvsPatchField.C"
    #include "calculatedFvsPatchField.H"
#endif


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// Runtime selection macros

#define makeFvsPatchTypeFieldTypeName(type)                                    \
                                                                               \
defineNamedTemplateTypeNameAndDebug(type, 0);

#define makeFvsPatchFieldsTypeName(type)                                       \
                                                                               \
makeFvsPatchTypeFieldTypeName(type##FvsPatchScalarField);                      \
makeFvsPatchTypeFieldTypeName(type##FvsPatchVectorField);                      \
makeFvsPatchTypeFieldTypeName(type##FvsPatchSphericalTensorField);             \
makeFvsPatchTypeFieldTypeName(type##FvsPatchSymmTensorField);                  \
makeFvsPatchTypeFieldTypeName(type##FvsPatchTensorField);

#define makeFvsPatchTypeField(PatchTypeField, typePatchTypeField)              \
                                                                               \
defineNamedTemplateTypeNameAndDebug(typePatchTypeField, 0);                    \
                                                                               \
addToRunTimeSelectionTable                                                     \
(                                                                              \
    PatchTypeField, typePatchTypeField, patch                                  \
);                                                                             \
                                                                               \
addToRunTimeSelectionTable                                                     \
(                                                                              \
    PatchTypeField,                                                            \
    typePatchTypeField,                                                        \
    patchMapper                                                                \
);                                                                             \
                                                                               \
addToRunTimeSelectionTable                                                     \
(                                                                              \
    PatchTypeField, typePatchTypeField, dictionary                             \
);


#define makeFvsPatchFields(type)                                               \
                                                                               \
makeFvsPatchTypeField(fvsPatchScalarField, type##FvsPatchScalarField);         \
makeFvsPatchTypeField(fvsPatchVectorField, type##FvsPatchVectorField);         \
makeFvsPatchTypeField                                                          \
(                                                                              \
    fvsPatchSphericalTensorField,                                              \
    type##FvsPatchSphericalTensorField                                         \
);                                                                             \
makeFvsPatchTypeField(fvsPatchSymmTensorField, type##FvsPatchSymmTensorField); \
makeFvsPatchTypeField(fvsPatchTensorField, type##FvsPatchTensorField);


#define makeFvsPatchTypeFieldTypedefs(type)                                    \
                                                                               \
typedef type##FvsPatchField<scalar> type##FvsPatchScalarField;                 \
typedef type##FvsPatchField<vector> type##FvsPatchVectorField;                 \
typedef type##FvsPatchField<sphericalTensor>                                   \
    type##FvsPatchSphericalTensorField;                                        \
typedef type##FvsPatchField<symmTensor> type##FvsPatchSymmTensorField;         \
typedef type##FvsPatchField<tensor> type##FvsPatchTensorField;


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
