/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2016 OpenFOAM Foundation
     \\/     M anipulation  |
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
#include "metisDecomp.H"
#include "floatScalar.H"
#include "Time.H"
#include "labelIOField.H"
#include "syncTools.H"
#include "globalIndex.H"
#include "dynamicLabelList.H"



extern "C"
{
#include <mpi.h>
#include <metis.h>
#include "parmetis.h"
}

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(parmetisDecomp, 0);
    addToRunTimeSelectionTable(decompositionMethod, parmetisDecomp, dictionary);
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

//- Does prevention of 0 cell domains and calls parmetis.
Foam::label Foam::parmetisDecomp::decompose
(
    Field<int>& xadj,
    Field<int>& adjncy,
    const pointField& cellCentres,
    Field<int>& cellWeights,
    Field<int>& faceWeights,
    const List<int>& options,
    List<int>& finalDecomp
)
{
    // C style numbering
    int numFlag = 0;

    // Number of dimensions
    int nDims = 3;


    if (cellCentres.size() != xadj.size()-1)
    {
        FatalErrorIn("parmetisDecomp::decompose(..)")
            << "cellCentres:" << cellCentres.size()
            << " xadj:" << xadj.size()
            << abort(FatalError);
    }


    // Get number of cells on all processors
    List<int> nLocalCells(Pstream::nProcs());
    nLocalCells[Pstream::myProcNo()] = xadj.size()-1;
    Pstream::gatherList(nLocalCells);
    Pstream::scatterList(nLocalCells);

    // Get cell offsets.
    List<int> cellOffsets(Pstream::nProcs()+1);
    int nGlobalCells = 0;
    forAll(nLocalCells, procI)
    {
        cellOffsets[procI] = nGlobalCells;
        nGlobalCells += nLocalCells[procI];
    }
    cellOffsets[Pstream::nProcs()] = nGlobalCells;

    // Convert pointField into the data type parmetis expects (float or double)
    Field<real_t> xyz(3*cellCentres.size());
    int compI = 0;
    forAll(cellCentres, cellI)
    {
        const point& cc = cellCentres[cellI];
        xyz[compI++] = float(cc.x());
        xyz[compI++] = float(cc.y());
        xyz[compI++] = float(cc.z());
    }

    // Make sure every domain has at least one cell
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // (Metis falls over with zero sized domains)
    // Trickle cells from processors that have them up to those that
    // don't.


    // Number of cells to send to the next processor
    // (is same as number of cells next processor has to receive)
    List<int> nSendCells(Pstream::nProcs(), 0);

    for (label procI = nLocalCells.size()-1; procI >=1; procI--)
    {
        if (nLocalCells[procI]-nSendCells[procI] < 1)
        {
            nSendCells[procI-1] = nSendCells[procI]-nLocalCells[procI]+1;
        }
    }

    // First receive (so increasing the sizes of all arrays)

    if (Pstream::myProcNo() >= 1 && nSendCells[Pstream::myProcNo()-1] > 0)
    {
        // Receive cells from previous processor
        IPstream fromPrevProc(Pstream::commsTypes::blocking, Pstream::myProcNo()-1);

        Field<int> prevXadj(fromPrevProc);
        Field<int> prevAdjncy(fromPrevProc);
        Field<real_t> prevXyz(fromPrevProc);
        Field<int> prevCellWeights(fromPrevProc);
        Field<int> prevFaceWeights(fromPrevProc);

        if (prevXadj.size() != nSendCells[Pstream::myProcNo()-1])
        {
            FatalErrorIn("parmetisDecomp::decompose(..)")
                << "Expected from processor " << Pstream::myProcNo()-1
                << " connectivity for " << nSendCells[Pstream::myProcNo()-1]
                << " nCells but only received " << prevXadj.size()
                << abort(FatalError);
        }

        // Insert adjncy
        prepend(prevAdjncy, adjncy);
        // Adapt offsets and prepend xadj
        xadj += prevAdjncy.size();
        prepend(prevXadj, xadj);
        // Coords
        prepend(prevXyz, xyz);
        // Weights
        prepend(prevCellWeights, cellWeights);
        prepend(prevFaceWeights, faceWeights);
    }


    // Send to my next processor

    if (nSendCells[Pstream::myProcNo()] > 0)
    {
        // Send cells to next processor
        OPstream toNextProc(Pstream::commsTypes::blocking, Pstream::myProcNo()+1);

        int nCells = nSendCells[Pstream::myProcNo()];
        int startCell = xadj.size()-1 - nCells;
        int startFace = xadj[startCell];
        int nFaces = adjncy.size()-startFace;

        // Send for all cell data: last nCells elements
        // Send for all face data: last nFaces elements
        toNextProc
            << Field<int>::subField(xadj, nCells, startCell)-startFace
            << Field<int>::subField(adjncy, nFaces, startFace)
            << SubField<real_t>(xyz, nDims*nCells, nDims*startCell)
            <<
            (
                cellWeights.size()
              ? static_cast<const Field<int>&>
                (
                    Field<int>::subField(cellWeights, nCells, startCell)
                )
              : Field<int>(0)
            )
            <<
            (
                faceWeights.size()
              ? static_cast<const Field<int>&>
                (
                    Field<int>::subField(faceWeights, nFaces, startFace)
                )
              : Field<int>(0)
            );

        // Remove data that has been sent
        if (faceWeights.size())
        {
            faceWeights.setSize(faceWeights.size()-nFaces);
        }
        if (cellWeights.size())
        {
            cellWeights.setSize(cellWeights.size()-nCells);
        }
        xyz.setSize(xyz.size()-nDims*nCells);
        adjncy.setSize(adjncy.size()-nFaces);
        xadj.setSize(xadj.size() - nCells);
    }



    // Adapt number of cells
    forAll(nSendCells, procI)
    {
        // Sent cells
        nLocalCells[procI] -= nSendCells[procI];

        if (procI >= 1)
        {
            // Received cells
            nLocalCells[procI] += nSendCells[procI-1];
        }
    }
    // Adapt cellOffsets
    nGlobalCells = 0;
    forAll(nLocalCells, procI)
    {
        cellOffsets[procI] = nGlobalCells;
        nGlobalCells += nLocalCells[procI];
    }


    if (nLocalCells[Pstream::myProcNo()] != (xadj.size()-1))
    {
        FatalErrorIn("parmetisDecomp::decompose(..)")
            << "Have connectivity for " << xadj.size()-1
            << " cells but nLocalCells:" << nLocalCells[Pstream::myProcNo()]
            << abort(FatalError);
    }

    // Weight info
    int wgtFlag = 0;
    int* vwgtPtr = NULL;
    int* adjwgtPtr = NULL;

    if (cellWeights.size())
    {
        vwgtPtr = cellWeights.begin();
        wgtFlag += 2;       // Weights on vertices
    }
    if (faceWeights.size())
    {
        adjwgtPtr = faceWeights.begin();
        wgtFlag += 1;       // Weights on edges
    }


    // Number of weights or balance constraints
    int nCon = 1;
    // Per processor, per constraint the weight
    Field<real_t> tpwgts(nCon*nDomains_, 1./nDomains_);
    // Imbalance tolerance
    Field<real_t> ubvec(nCon, 1.02);
    if (nDomains_ == 1)
    {
        // If only one processor there is no imbalance.
        ubvec[0] = 1;
    }

    MPI_Comm comm = MPI_COMM_WORLD;

    // output: cell -> processor addressing
    finalDecomp.setSize(nLocalCells[Pstream::myProcNo()]);

    // output: number of cut edges
    int edgeCut = 0;


    ParMETIS_V3_PartGeomKway
    (
        cellOffsets.begin(),    // vtxDist
        xadj.begin(),
        adjncy.begin(),
        vwgtPtr,                // vertexweights
        adjwgtPtr,              // edgeweights
        &wgtFlag,
        &numFlag,
        &nDims,
        xyz.begin(),
        &nCon,
        &nDomains_,          // nParts
        tpwgts.begin(),
        ubvec.begin(),
        const_cast<List<int>&>(options).begin(),
        &edgeCut,
        finalDecomp.begin(),
        &comm
    );


    // If we sent cells across make sure we undo it
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // Receive back from next processor if I sent something
    if (nSendCells[Pstream::myProcNo()] > 0)
    {
        IPstream fromNextProc(Pstream::commsTypes::blocking, Pstream::myProcNo()+1);

        List<int> nextFinalDecomp(fromNextProc);

        if (nextFinalDecomp.size() != nSendCells[Pstream::myProcNo()])
        {
            FatalErrorIn("parmetisDecomp::decompose(..)")
                << "Expected from processor " << Pstream::myProcNo()+1
                << " decomposition for " << nSendCells[Pstream::myProcNo()]
                << " nCells but only received " << nextFinalDecomp.size()
                << abort(FatalError);
        }

        append(nextFinalDecomp, finalDecomp);
    }

    // Send back to previous processor.
    if (Pstream::myProcNo() >= 1 && nSendCells[Pstream::myProcNo()-1] > 0)
    {
        OPstream toPrevProc(Pstream::commsTypes::blocking, Pstream::myProcNo()-1);

        int nToPrevious = nSendCells[Pstream::myProcNo()-1];

        toPrevProc <<
            SubList<int>
            (
                finalDecomp,
                nToPrevious,
                finalDecomp.size()-nToPrevious
            );

        // Remove locally what has been sent
        finalDecomp.setSize(finalDecomp.size()-nToPrevious);
    }

    return edgeCut;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::parmetisDecomp::parmetisDecomp
(
    const dictionary& decompRegionDict
    //   const polyMesh& mesh
)
:
    decompositionMethod(decompRegionDict)
    //  mesh_(mesh)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::labelList Foam::parmetisDecomp::decompose
(
    const polyMesh& mesh,
    const pointField& cc,
    const scalarField& cWeights
)
{
    if (cc.size() != mesh.nCells())
    {
        FatalErrorIn
        (
            "parmetisDecomp::decompose"
            "(const pointField&, const scalarField&)"
        )   << "Can use this decomposition method only for the whole mesh"
            << endl
            << "and supply one coordinate (cellCentre) for every cell." << endl
            << "The number of coordinates " << cc.size() << endl
            << "The number of cells in the mesh " << mesh.nCells()
            << exit(FatalError);
    }

    // For running sequential ...
    if (Pstream::nProcs() <= 1)
    {
        return metisDecomp(decompRegionDict_).decompose
        (
            mesh,
            cc,
            cWeights
        );
    }


    // Connections
    Field<int> adjncy;
    // Offsets into adjncy
    Field<int> xadj;
    calcDistributedCSR
    (
        mesh,
        adjncy,
        xadj
    );


    // decomposition options. 0 = use defaults
    List<int> options(3, 0);
    //options[0] = 1;     // don't use defaults but use values below
    //options[1] = -1;    // full debug info
    //options[2] = 15;    // random number seed

    // cell weights (so on the vertices of the dual)
    Field<int> cellWeights;

    // face weights (so on the edges of the dual)
    Field<int> faceWeights;


    // Check for externally provided cellweights and if so initialise weights
    scalar minWeights = gMin(cWeights);
    if (cWeights.size() > 0)
    {
        if (minWeights <= 0)
        {
            WarningIn
            (
                "metisDecomp::decompose"
                "(const pointField&, const scalarField&)"
            )   << "Illegal minimum weight " << minWeights
                << endl;
        }

        if (cWeights.size() != mesh.nCells())
        {
            FatalErrorIn
            (
                "parmetisDecomp::decompose"
                "(const pointField&, const scalarField&)"
            )   << "Number of cell weights " << cWeights.size()
                << " does not equal number of cells " << mesh.nCells()
                << exit(FatalError);
        }

        // Convert to integers.
        cellWeights.setSize(cWeights.size());
        forAll(cellWeights, i)
        {
            cellWeights[i] = int(cWeights[i]/minWeights);
        }
    }


    // Check for user supplied weights and decomp options
    if (decompRegionDict_.found("metisCoeffs"))
    {
        const dictionary& metisCoeffs =
            decompRegionDict_.subDict("metisCoeffs");
        word weightsFile;

        if (metisCoeffs.readIfPresent("cellWeightsFile", weightsFile))
        {
            Info<< "parmetisDecomp : Using cell-based weights read from "
                << weightsFile << endl;

            labelIOField cellIOWeights
            (
                IOobject
                (
                    weightsFile,
                    mesh.time().timeName(),
                    mesh,
                    IOobject::MUST_READ,
                    IOobject::AUTO_WRITE
                )
            );
            cellWeights.transfer(cellIOWeights);

            if (cellWeights.size() != mesh.nCells())
            {
                FatalErrorIn
                (
                    "parmetisDecomp::decompose"
                    "(const pointField&, const scalarField&)"
                )   << "Number of cell weights " << cellWeights.size()
                    << " read from " << cellIOWeights.objectPath()
                    << " does not equal number of cells " << mesh.nCells()
                    << exit(FatalError);
            }
        }

        if (metisCoeffs.readIfPresent("faceWeightsFile", weightsFile))
        {
            Info<< "parmetisDecomp : Using face-based weights read from "
                << weightsFile << endl;

            labelIOField weights
            (
                IOobject
                (
                    weightsFile,
                    mesh.time().timeName(),
                    mesh,
                    IOobject::MUST_READ,
                    IOobject::AUTO_WRITE
                )
            );

            if (weights.size() != mesh.nFaces())
            {
                FatalErrorIn
                (
                    "parmetisDecomp::decompose"
                    "(const pointField&, const scalarField&)"
                )   << "Number of face weights " << weights.size()
                    << " does not equal number of internal and boundary faces "
                    << mesh.nFaces()
                    << exit(FatalError);
            }

            faceWeights.setSize(adjncy.size());

            // Assume symmetric weights. Keep same ordering as adjncy.
            List<int> nFacesPerCell(mesh.nCells(), 0);

            // Handle internal faces
            for (label faceI = 0; faceI < mesh.nInternalFaces(); faceI++)
            {
                label w = weights[faceI];

                label own = mesh.faceOwner()[faceI];
                label nei = mesh.faceNeighbour()[faceI];

                faceWeights[xadj[own] + nFacesPerCell[own]++] = w;
                faceWeights[xadj[nei] + nFacesPerCell[nei]++] = w;
            }
            // Coupled boundary faces
            const polyBoundaryMesh& patches = mesh.boundaryMesh();

            forAll(patches, patchI)
            {
                const polyPatch& pp = patches[patchI];

                if (pp.coupled())
                {
                    label faceI = pp.start();

                    forAll(pp, i)
                    {
                        label w = weights[faceI];
                        label own = mesh.faceOwner()[faceI];
                        faceWeights[xadj[own] + nFacesPerCell[own]++] = w;
                        faceI++;
                    }
                }
            }
        }

        if (metisCoeffs.readIfPresent("options", options))
        {
            Info<< "Using Metis options     " << options
                << nl << endl;

            if (options.size() != 3)
            {
                FatalErrorIn
                (
                    "parmetisDecomp::decompose"
                    "(const pointField&, const scalarField&)"
                )   << "Number of options " << options.size()
                    << " should be three." << exit(FatalError);
            }
        }
    }


    // Do actual decomposition
    List<int> finalDecomp;
    decompose
    (
        xadj,
        adjncy,
        cc,
        cellWeights,
        faceWeights,
        options,
        finalDecomp
    );

    // Copy back to labelList
    labelList decomp(finalDecomp.size());

    forAll(decomp, i)
    {
        decomp[i] = finalDecomp[i];
    }


    return decomp;
}


Foam::labelList Foam::parmetisDecomp::decompose
(
    const polyMesh& mesh,
    const labelList& cellToRegion,
    const pointField& regionPoints,
    const scalarField& regionWeights
)
{
    const labelList& faceOwner = mesh.faceOwner();
    const labelList& faceNeighbour = mesh.faceNeighbour();
    const polyBoundaryMesh& patches = mesh.boundaryMesh();

    if (cellToRegion.size() != mesh.nCells())
    {
        FatalErrorIn
        (
            "parmetisDecomp::decompose(const labelList&, const pointField&)"
        )   << "Size of cell-to-coarse map " << cellToRegion.size()
            << " differs from number of cells in mesh " << mesh.nCells()
            << exit(FatalError);
    }


    // Global region numbering engine
    globalIndex globalRegions(regionPoints.size());


    // Get renumbered owner region on other side of coupled faces
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    List<int> globalNeighbour(mesh.nFaces()-mesh.nInternalFaces());

    forAll(patches, patchI)
    {
        const polyPatch& pp = patches[patchI];

        if (pp.coupled())
        {
            label faceI = pp.start();
            label bFaceI = pp.start() - mesh.nInternalFaces();

            forAll(pp, i)
            {
                label ownRegion = cellToRegion[faceOwner[faceI]];
                globalNeighbour[bFaceI++] = globalRegions.toGlobal(ownRegion);
                faceI++;
            }
        }
    }

    // Get the cell on the other side of coupled patches
    syncTools::swapBoundaryFaceList(mesh, globalNeighbour);


    // Get globalCellCells on coarse mesh
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    labelListList globalRegionRegions;
    {
        List<dynamicLabelList > dynRegionRegions(regionPoints.size());

        // Internal faces first
        forAll(faceNeighbour, faceI)
        {
            label ownRegion = cellToRegion[faceOwner[faceI]];
            label neiRegion = cellToRegion[faceNeighbour[faceI]];

            if (ownRegion != neiRegion)
            {
                label globalOwn = globalRegions.toGlobal(ownRegion);
                label globalNei = globalRegions.toGlobal(neiRegion);

                if (findIndex(dynRegionRegions[ownRegion], globalNei) == -1)
                {
                    dynRegionRegions[ownRegion].append(globalNei);
                }
                if (findIndex(dynRegionRegions[neiRegion], globalOwn) == -1)
                {
                    dynRegionRegions[neiRegion].append(globalOwn);
                }
            }
        }

        // Coupled boundary faces
        forAll(patches, patchI)
        {
            const polyPatch& pp = patches[patchI];

            if (pp.coupled())
            {
                label faceI = pp.start();
                label bFaceI = pp.start() - mesh.nInternalFaces();

                forAll(pp, i)
                {
                    label ownRegion = cellToRegion[faceOwner[faceI]];
                    label globalNei = globalNeighbour[bFaceI++];
                    faceI++;

                    if
                    (
                        findIndex(dynRegionRegions[ownRegion], globalNei)
                     == -1
                    )
                    {
                        dynRegionRegions[ownRegion].append(globalNei);
                    }
                }
            }
        }

        globalRegionRegions.setSize(dynRegionRegions.size());
        forAll(dynRegionRegions, i)
        {
            globalRegionRegions[i].transfer(dynRegionRegions[i]);
        }
    }

    labelList regionDecomp
    (
        decompose
        (
            globalRegionRegions,
            regionPoints,
            regionWeights
        )
    );

    // Rework back into decomposition for original mesh
    labelList cellDistribution(cellToRegion.size());

    forAll(cellDistribution, cellI)
    {
        cellDistribution[cellI] = regionDecomp[cellToRegion[cellI]];
    }


    return cellDistribution;
}


Foam::labelList Foam::parmetisDecomp::decompose
(
    const labelListList& globalCellCells,
    const pointField& cellCentres,
    const scalarField& cWeights
)
{
    if (cellCentres.size() != globalCellCells.size())
    {
        FatalErrorIn
        (
            "parmetisDecomp::decompose(const labelListList&"
            ", const pointField&, const scalarField&)"
        )   << "Inconsistent number of cells (" << globalCellCells.size()
            << ") and number of cell centres (" << cellCentres.size()
            << ") or weights (" << cWeights.size() << ")." << exit(FatalError);
    }

    // For running sequential ...
    if (Pstream::nProcs() <= 1)
    {
        return metisDecomp(decompRegionDict_).decompose
        (
            globalCellCells,
            cellCentres,
            cWeights
        );
    }


    // Make Metis Distributed CSR (Compressed Storage Format) storage

    // Connections
    Field<int> adjncy;

    // Offsets into adjncy
    Field<int> xadj;

    calcCSR(globalCellCells, adjncy, xadj);

    // decomposition options. 0 = use defaults
    List<int> options(3, 0);
    //options[0] = 1;     // don't use defaults but use values below
    //options[1] = -1;    // full debug info
    //options[2] = 15;    // random number seed

    // cell weights (so on the vertices of the dual)
    Field<int> cellWeights;

    // face weights (so on the edges of the dual)
    Field<int> faceWeights;


    // Check for externally provided cellweights and if so initialise weights
    scalar minWeights = gMin(cWeights);
    if (cWeights.size() > 0)
    {
        if (minWeights <= 0)
        {
            WarningIn
            (
                "parmetisDecomp::decompose(const labelListList&"
                ", const pointField&, const scalarField&)"
            )   << "Illegal minimum weight " << minWeights
                << endl;
        }

        if (cWeights.size() != globalCellCells.size())
        {
            FatalErrorIn
            (
                "parmetisDecomp::decompose(const labelListList&"
                ", const pointField&, const scalarField&)"
            )   << "Number of cell weights " << cWeights.size()
                << " does not equal number of cells " << globalCellCells.size()
                << exit(FatalError);
        }

        // Convert to integers.
        cellWeights.setSize(cWeights.size());
        forAll(cellWeights, i)
        {
            cellWeights[i] = int(cWeights[i]/minWeights);
        }
    }


    // Check for user supplied weights and decomp options
    if (decompRegionDict_.found("metisCoeffs"))
    {
        const dictionary& metisCoeffs =
            decompRegionDict_.subDict("metisCoeffs");

        if (metisCoeffs.readIfPresent("options", options))
        {
            Info<< "Using Metis options     " << options
                << nl << endl;

            if (options.size() != 3)
            {
                FatalErrorIn
                (
                    "parmetisDecomp::decompose(const labelListList&"
                    ", const pointField&, const scalarField&)"
                )   << "Number of options " << options.size()
                    << " should be three." << exit(FatalError);
            }
        }
    }


    // Do actual decomposition
    List<int> finalDecomp;
    decompose
    (
        xadj,
        adjncy,
        cellCentres,
        cellWeights,
        faceWeights,
        options,
        finalDecomp
    );

    // Copy back to labelList
    labelList decomp(finalDecomp.size());

    forAll(decomp, i)
    {
        decomp[i] = finalDecomp[i];
    }

    return decomp;
}


// ************************************************************************* //
