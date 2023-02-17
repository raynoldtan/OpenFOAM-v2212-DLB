/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2015 OpenFOAM Foundation
    Copyright (C) 2018-2021 OpenCFD Ltd.
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

#include "parmetisDecomp.H"
#include "addToRunTimeSelectionTable.H"
#include "Time.H"

static const char* notImplementedMessage =
"Attempted to use <parmetis> without the parmetisDecomp library loaded.\n"
"This message is from the dummy parmetisDecomp stub library instead.\n\n"
"Please install <ptscotch> and ensure libptscotch.so is in LD_LIBRARY_PATH.\n"
"The parmetisDecomp library can then be built from "
"src/parallel/decompose/parmetisDecomp.\n"
"Dynamically loading or linking this library will add "
"<parmetis> as a decomposition method.\n";


// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(parmetisDecomp, 0);
    addToRunTimeSelectionTable
    (
        decompositionMethod,
        parmetisDecomp,
        dictionary
    );
}


// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

Foam::label Foam::parmetisDecomp::decompose
(
    Field<int>& xadj,
    Field<int>& adjncy,
    const pointField& cellCentres,
    Field<int>& cellWeights,
    Field<int>& faceWeights,
    const List<int>& options,    
    labelList& finalDecomp
)
{
    FatalErrorInFunction
        << notImplementedMessage << exit(FatalError);

    return -1;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::parmetisDecomp::parmetisDecomp
(
    const dictionary& decompDict,
    const word& regionName
)
:
    decompositionMethod(decompDict, regionName)
    
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::labelList Foam::parmetisDecomp::decompose
(
    const polyMesh& mesh,
    const pointField& points,
    const scalarField& pointWeights
)
{
    FatalErrorInFunction
        << notImplementedMessage << exit(FatalError);

    return labelList();
}


Foam::labelList Foam::parmetisDecomp::decompose
(
    const polyMesh& mesh,
    const labelList& agglom,
    const pointField& agglomPoints,
    const scalarField& pointWeights
) 
{
    FatalErrorInFunction
        << notImplementedMessage << exit(FatalError);

    return labelList();
}


Foam::labelList Foam::parmetisDecomp::decompose
(
    const labelListList& globalCellCells,
    const pointField& cellCentres,
    const scalarField& cWeights
) 
{
    FatalErrorInFunction
        << notImplementedMessage << exit(FatalError);

    return labelList();
}


// ************************************************************************* //
