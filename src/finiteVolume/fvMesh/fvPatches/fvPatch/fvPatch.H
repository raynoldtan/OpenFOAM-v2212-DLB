/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2018 OpenFOAM Foundation
    Copyright (C) 2020-2022 OpenCFD Ltd.
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
    Foam::fvPatch

Description
    A finiteVolume patch using a polyPatch and a fvBoundaryMesh

SourceFiles
    fvPatch.C
    fvPatchNew.C

\*---------------------------------------------------------------------------*/

#ifndef Foam_fvPatch_H
#define Foam_fvPatch_H

#include "polyPatch.H"
#include "labelList.H"
#include "SubList.H"
#include "SubField.H"
#include "PtrList.H"
#include "typeInfo.H"
#include "autoPtr.H"
#include "tmp.H"
#include "primitiveFields.H"
#include "fvPatchFieldsFwd.H"
#include "runTimeSelectionTables.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
class fvBoundaryMesh;
class fvPatch;
class surfaceInterpolation;

//- Store lists of fvPatch as a PtrList
typedef PtrList<fvPatch> fvPatchList;

/*---------------------------------------------------------------------------*\
                           Class fvPatch Declaration
\*---------------------------------------------------------------------------*/

class fvPatch
{
    // Private Data

        //- Reference to the underlying polyPatch
        const polyPatch& polyPatch_;

        //- Reference to boundary mesh
        const fvBoundaryMesh& boundaryMesh_;


    // Private Member Functions

        //- No copy construct
        fvPatch(const fvPatch&) = delete;

        //- No copy assignment
        void operator=(const fvPatch&) = delete;


public:

    // Protected Member Functions

        //- Make patch weighting factors
        virtual void makeWeights(scalarField&) const;

        //- Correct patch deltaCoeffs
        virtual void makeDeltaCoeffs(scalarField&) const;

        //- Correct patch non-ortho deltaCoeffs
        virtual void makeNonOrthoDeltaCoeffs(scalarField&) const;

        //- Correct patch non-ortho correction vectors
        virtual void makeNonOrthoCorrVectors(vectorField&) const;

        //- Initialise the patches for moving points
        virtual void initMovePoints();

        //- Correct patches after moving points
        virtual void movePoints();


public:

    //- The boundary type associated with the patch
    typedef fvBoundaryMesh BoundaryMesh;

    friend class fvBoundaryMesh;
    friend class surfaceInterpolation;

    //- Runtime type information
    TypeName(polyPatch::typeName_());


    // Declare run-time constructor selection tables

        declareRunTimeSelectionTable
        (
            autoPtr,
            fvPatch,
            polyPatch,
            (const polyPatch& patch, const fvBoundaryMesh& bm),
            (patch, bm)
        );


    // Constructors

        //- Construct from polyPatch and fvBoundaryMesh
        fvPatch(const polyPatch&, const fvBoundaryMesh&);


    // Selectors

        //- Return a pointer to a new patch created on freestore from polyPatch
        static autoPtr<fvPatch> New
        (
            const polyPatch&,
            const fvBoundaryMesh&
        );


    //- Destructor
    virtual ~fvPatch();


    // Member Functions

        //- Lookup the polyPatch index on corresponding fvMesh
        //  \note Fatal if the polyPatch is not associated with a fvMesh
        static const fvPatch& lookupPatch(const polyPatch& p);


    // Access

            //- Return the polyPatch
            const polyPatch& patch() const noexcept
            {
                return polyPatch_;
            }

            //- Return name
            virtual const word& name() const
            {
                return polyPatch_.name();
            }

            //- Return start label of this patch in the polyMesh face list
            virtual label start() const
            {
                return polyPatch_.start();
            }

            //- Return size
            virtual label size() const
            {
                return polyPatch_.size();
            }

            //- Return true if this patch is coupled
            virtual bool coupled() const
            {
                return polyPatch_.coupled();
            }

            //- Return true if the given type is a constraint type
            static bool constraintType(const word& patchType);

            //- Return a list of all the constraint patch types
            static wordList constraintTypes();

            //- Return the index of this patch in the fvBoundaryMesh
            label index() const noexcept
            {
                return polyPatch_.index();
            }

            //- Return boundaryMesh reference
            const fvBoundaryMesh& boundaryMesh() const noexcept
            {
                return boundaryMesh_;
            }

            //- Slice List to patch, using the virtual patch size
            template<class T>
            const typename List<T>::subList patchSlice(const List<T>& l) const
            {
                return typename List<T>::subList(l, size(), start());
            }

            //- Slice List to patch, using the underlying polyPatch information
            template<class T>
            const typename List<T>::subList patchRawSlice
            (
                const List<T>& l
            ) const
            {
                return typename List<T>::subList
                (
                    l,
                    polyPatch_.size(),
                    polyPatch_.start()
                );
            }

            //- Return faceCells
            virtual const labelUList& faceCells() const;


        // Access functions for geometrical data

            //- Return face centres
            const vectorField& Cf() const;

            //- Return neighbour cell centres
            tmp<vectorField> Cn() const;

            //- Return face area vectors
            const vectorField& Sf() const;

            //- Return face area magnitudes
            const scalarField& magSf() const;

            //- Return face normals
            tmp<vectorField> nf() const;

            //- Return cell-centre to face-centre vector
            //- except for coupled patches for which the cell-centre
            //- to coupled-cell-centre vector is returned
            virtual tmp<vectorField> delta() const;


        // Access functions for demand driven data

            //- Return patch weighting factors
            const scalarField& weights() const;

            //- Return the face - cell distance coefficient
            //- except for coupled patches for which the cell-centre
            //- to coupled-cell-centre distance coefficient is returned
            const scalarField& deltaCoeffs() const;


        // Evaluation Functions

            //- Extract internal field next to patch using faceCells mapping
            template<class Type>
            inline void patchInternalField
            (
                const UList<Type>& f,
                const labelUList& faceCells,
                Field<Type>& pfld
            ) const;

            //- Return given internal field next to patch as patch field
            template<class Type>
            tmp<Field<Type>> patchInternalField(const UList<Type>&) const;

            //- Return given internal field next to patch as patch field
            //- using provided addressing
            template<class Type>
            tmp<Field<Type>> patchInternalField
            (
                const UList<Type>& f,
                const labelUList& faceCells
            ) const;

            //- Return given internal field next to patch as patch field
            template<class Type>
            void patchInternalField(const UList<Type>&, Field<Type>&) const;

            //- Return the patch field of the GeometricField
            //- corresponding to this patch.
            template<class GeometricField, class AnyType = bool>
            const typename GeometricField::Patch& patchField
            (
                const GeometricField& gf
            ) const;

            //- Lookup the named field from the local registry and
            //- return the patch field corresponding to this patch.
            //  N.B.  The dummy pointer arguments are used if this function is
            //  instantiated within a templated function to avoid a bug in gcc.
            template<class GeometricField, class AnyType = bool>
            const typename GeometricField::Patch& lookupPatchField
            (
                const word& name,
                const GeometricField* = nullptr,
                const AnyType* = nullptr
            ) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "fvPatchTemplates.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
