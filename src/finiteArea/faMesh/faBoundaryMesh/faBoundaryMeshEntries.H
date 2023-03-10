/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

Class
    Foam::faBoundaryMeshEntries

Description
    Read and store dictionary entries for finite-area boundary patches.

SourceFiles
    faBoundaryMeshEntries.C

\*---------------------------------------------------------------------------*/

#ifndef Foam_faBoundaryMeshEntries_H
#define Foam_faBoundaryMeshEntries_H

#include "regIOobject.H"
#include "PtrList.H"
#include "entry.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                    Class faBoundaryMeshEntries Declaration
\*---------------------------------------------------------------------------*/

class faBoundaryMeshEntries
:
    public regIOobject,
    public PtrList<entry>
{
public:

    //- Runtime type information
    TypeNameNoDebug("faBoundaryMesh");


    // Constructors

        //- Read construct from IOobject
        explicit faBoundaryMeshEntries(const IOobject& io)
        :
            regIOobject(io),
            PtrList<entry>()
        {
            if (isReadRequired() || (isReadOptional() && headerOk()))
            {
                readStream(typeName) >> *this;
            }
        }


   // Member Functions

        bool writeData(Ostream&) const
        {
            NotImplemented;
            return false;
        }
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
