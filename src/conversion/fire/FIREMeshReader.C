/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2016 OpenCFD Ltd.
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

#include "FIREMeshReader.H"
#include "wallPolyPatch.H"
#include "ListOps.H"
#include "IFstream.H"


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

Foam::word Foam::fileFormats::FIREMeshReader::validateWord
(
    const std::string& str
)
{
    std::string::size_type ngood = 0;
    bool prefix = false;
    bool first  = true;

    for
    (
        std::string::const_iterator iter = str.begin();
        iter != str.end();
        ++iter
    )
    {
        if (word::valid(*iter))
        {
            ++ngood;
            if (first)
            {
                first = false;

                // start with a digit? need to prefix with '_'
                if (isdigit(*iter))
                {
                    prefix = true;
                }
            }
        }
    }

    if (prefix)
    {
        ++ngood;
    }
    else if (ngood == str.size())
    {
        return str;
    }

    Foam::word out;
    out.resize(ngood);
    ngood = 0;

    Foam::word::iterator iter2 = out.begin();
    for
    (
        std::string::const_iterator iter1 = str.begin();
        iter1 != str.end();
        ++iter1
    )
    {
        register char c = *iter1;

        if (Foam::word::valid(c))
        {
            if (prefix)
            {
                prefix = false;
                *(iter2++) = '_';
                ++ngood;
            }
            *(iter2++) = c;
            ++ngood;
        }
    }

    out.resize(ngood);

    return out;
}


void Foam::fileFormats::FIREMeshReader::readPoints
(
    ISstream& is,
    const scalar scaleFactor
)
{
    const label n = FIRECore::readPoints(is, points_);
    // the above has FatalError if there are no points

    Info<< "Number of points = " << n << endl;
    if (scaleFactor > 1.0 + SMALL || scaleFactor < 1.0 - SMALL)
    {
        points_ *= scaleFactor;
    }
}


void Foam::fileFormats::FIREMeshReader::readFaces(ISstream& is)
{
    const label nFaces = getFireLabel(is);
    Info<< "Number of faces  = " << nFaces << endl;
    meshFaces_.setSize(nFaces);

    if (nFaces > 0)
    {
        forAll(meshFaces_, faceI)
        {
            const label size = getFireLabel(is);

            face& f = meshFaces_[faceI];
            f.setSize(size);
            forAll(f, fp)
            {
                f[fp] = getFireLabel(is);
            }

            // flip in-place
            f.flip();
        }
    }
    else
    {
        FatalErrorInFunction
            << "no faces in file " << is.name()
            << abort(FatalError);
    }
}


void Foam::fileFormats::FIREMeshReader::readCells(ISstream& is)
{
    const label nCells = getFireLabel(is);
    Info<< "Number of cells  = " << nCells << endl;

    owner_.setSize(meshFaces_.size());
    neigh_.setSize(meshFaces_.size());

    owner_ = -1;
    neigh_ = -1;

    if (nCells > 0)
    {
        for (label cellI = 0; cellI < nCells; ++cellI)
        {
            const label nface = getFireLabel(is);

            for (label i = 0; i < nface; ++i)
            {
                const label faceI = getFireLabel(is);

                if (owner_[faceI] == -1)
                {
                    owner_[faceI] = cellI;
                }
                else if (neigh_[faceI] == -1)
                {
                    neigh_[faceI] = cellI;
                }
                else
                {
                    Warning
                        << "bad cell connectivity for face " << faceI
                        << " on cell " << cellI
                        << endl;
                }
            }
        }
    }
    else
    {
        FatalErrorInFunction
            << "no cells in file " << is.name()
            << abort(FatalError);
    }

    cellTableId_.setSize(nCells);
    cellTableId_ = -1;
}


void Foam::fileFormats::FIREMeshReader::readSelections(ISstream& is)
{
    const label nSelect = getFireLabel(is);
    Info<< "Number of select = " << nSelect << endl;

    label nCellSelections = 0;
    label nFaceSelections = 0;

    faceZoneId_.setSize(meshFaces_.size());
    faceZoneId_ = -1;

    DynamicList<word> faceNames(32);

    for (label selI = 0; selI < nSelect; ++selI)
    {
        std::string name    = getFireString(is);
        const label selType = getFireLabel(is);
        const label count   = getFireLabel(is);

        if (selType == FIRECore::cellSelection)
        {
            // index starting at 1
            const label selId = ++nCellSelections;

            cellTable_.setName(selId, validateWord(name));
            cellTable_.setMaterial(selId, "fluid");

            for (label i = 0; i < count; ++i)
            {
                const label cellId = getFireLabel(is);

                cellTableId_[cellId] = selId;
            }
        }
        else if (selType == FIRECore::faceSelection)
        {
            // index starting at 0
            const label selId = nFaceSelections++;

            faceNames.append(validateWord(name));

            for (label i = 0; i < count; ++i)
            {
                const label faceId = getFireLabel(is);

                faceZoneId_[faceId] = selId;
            }
        }
        else
        {
            // discard other selection types (eg, nodes)
            for (label i = 0; i < count; ++i)
            {
                getFireLabel(is);
            }
        }
    }

    Info<< nFaceSelections << " face selections" << endl;
    Info<< nCellSelections << " cell selections" << endl;

    // add extra for missed boundary faces
    faceNames.append("__MISSED_FACES__");
    faceNames_.transfer(faceNames);
}


void Foam::fileFormats::FIREMeshReader::reorganize()
{
    nInternalFaces_ = 0;

    // pass 1:
    // count internal faces and also swap owner <-> neigh as required
    forAll(meshFaces_, faceI)
    {
        if (neigh_[faceI] != -1)
        {
            ++nInternalFaces_;

            if (owner_[faceI] > neigh_[faceI])
            {
                Swap(owner_[faceI], neigh_[faceI]);
            }
        }
    }

    label posInternal = 0;
    label posExternal = nInternalFaces_;

    labelList oldToNew(meshFaces_.size(), -1);

    // pass 2:
    // mapping to ensure proper division of internal / external
    forAll(meshFaces_, faceI)
    {
        if (neigh_[faceI] == -1)
        {
            oldToNew[faceI] = posExternal++;
        }
        else
        {
            oldToNew[faceI] = posInternal++;
        }
    }

    inplaceReorder(oldToNew, meshFaces_);
    inplaceReorder(oldToNew, owner_);
    inplaceReorder(oldToNew, neigh_);
    inplaceReorder(oldToNew, faceZoneId_);

    // determine the patch sizes - faceNames_ already has extra place for missed faces
    const label zoneMissed = faceNames_.size() - 1;
    patchSizes_.setSize(faceNames_.size());
    patchSizes_ = 0;

    patchStarts_.setSize(patchSizes_.size());
    patchStarts_ = 0;

    for (label faceI = nInternalFaces_; faceI < meshFaces_.size(); ++faceI)
    {
        label zoneI = faceZoneId_[faceI];
        if (zoneI == -1)
        {
            ++patchSizes_[zoneMissed];
        }
        else
        {
            ++patchSizes_[zoneI];
        }
    }

    if (patchSizes_[zoneMissed])
    {
        Info<<"collecting " << patchSizes_[zoneMissed]
            << " missed boundary faces to final patch" << endl;
    }

    oldToNew = -1;

    // calculate the patch starts
    {
        label pos = nInternalFaces_;

        forAll(patchStarts_, patchI)
        {
            patchStarts_[patchI] = pos;
            pos += patchSizes_[patchI];
        }

        forAll(patchSizes_, patchI)
        {
            patchSizes_[patchI] = 0;
        }
    }

    // reordering
    for (label faceI = nInternalFaces_; faceI < meshFaces_.size(); ++faceI)
    {
        label patchI = faceZoneId_[faceI];
        if (patchI == -1)
        {
            oldToNew[faceI] = patchStarts_[zoneMissed] + patchSizes_[zoneMissed];
            ++patchSizes_[zoneMissed];
        }
        else
        {
            oldToNew[faceI] = patchStarts_[patchI] + patchSizes_[patchI];
            ++patchSizes_[patchI];
        }
    }

    // discard old information
    faceZoneId_.clear();

    inplaceReorder(oldToNew, meshFaces_);
    inplaceReorder(oldToNew, owner_);
    inplaceReorder(oldToNew, neigh_);

    //--- neigh_.setSize(nInternalFaces_);

    // finally reduce to the number of patches actually used
    patchNames_.setSize(patchSizes_.size());
    oldToNew = -1;

    label nPatches = 0;
    forAll(patchSizes_, patchI)
    {
        if (patchSizes_[patchI])
        {
            patchNames_[nPatches] = faceNames_[patchI];

            oldToNew[patchI] = nPatches;
            ++nPatches;
        }
    }

    inplaceReorder(oldToNew, patchStarts_);
    inplaceReorder(oldToNew, patchSizes_);

    patchStarts_.setSize(nPatches);
    patchSizes_.setSize(nPatches);
    patchNames_.setSize(nPatches);
}


void Foam::fileFormats::FIREMeshReader::addPatches(polyMesh& mesh) const
{
    // create patches
    List<polyPatch*> newPatches(patchSizes_.size());

    label meshFaceI = nInternalFaces_;

    forAll(patchStarts_, patchI)
    {
        Info<< "patch " << patchI
            << " (start: " << meshFaceI << " size: " << patchSizes_[patchI]
            << ") name: " << patchNames_[patchI]
            << endl;

        // don't know anything better - just make it a wall
        newPatches[patchI] = new polyPatch
        (
            patchNames_[patchI],
            patchSizes_[patchI],
            meshFaceI,
            patchI,
            mesh.boundaryMesh(),
            word::null
        );

        meshFaceI += patchSizes_[patchI];
    }

    mesh.addPatches(newPatches);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

bool Foam::fileFormats::FIREMeshReader::readGeometry(const scalar scaleFactor)
{
    IOstream::streamFormat fmt = IOstream::ASCII;

    const word ext = geometryFile_.ext();
    bool supported = FIRECore::file3dExtensions.found(ext);
    if (supported)
    {
        FIRECore::fileExt3d fireFileType = FIRECore::file3dExtensions[ext];
        if (fireFileType == FIRECore::POLY_ASCII)
        {
            fmt = IOstream::ASCII;
        }
        else if (fireFileType == FIRECore::POLY_BINARY)
        {
            fmt = IOstream::BINARY;
        }
        else
        {
            // compressed or something
            supported = false;
        }
    }

    if (!supported)
    {
        FatalErrorInFunction
            << "File-type '" << ext
            << "' is not supported for reading as a FIRE mesh." << nl
            << "If it is a compressed file, use gunzip first."
            << abort(FatalError);
    }

    IFstream is(geometryFile_, fmt, false);

    readPoints(is, scaleFactor);
    readFaces(is);
    readCells(is);
    readSelections(is);

    return true;
}

Foam::autoPtr<Foam::polyMesh> Foam::fileFormats::FIREMeshReader::mesh
(
    const objectRegistry& registry
)
{
    readGeometry(scaleFactor_);
    reorganize();

    Info<< "Creating a polyMesh" << endl;

    autoPtr<polyMesh> mesh
    (
        new polyMesh
        (
            IOobject
            (
                polyMesh::defaultRegion,
                "constant",
                registry
            ),
            xferMove(points_),
            xferMove(meshFaces_),
            xferMove(owner_),
            xferMove(neigh_)
        )
    );

    // adding patches also checks the mesh
    addPatches(mesh());

    cellTable_.addCellZones(mesh(), cellTableId_);

    // addFaceZones(mesh());

    return mesh;
}

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::fileFormats::FIREMeshReader::FIREMeshReader
(
    const fileName& name,
    const scalar scaleFactor
)
:
    meshReader(name, scaleFactor),
    owner_(0),
    neigh_(0),
    faceZoneId_(0),
    faceNames_()
{}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::fileFormats::FIREMeshReader::~FIREMeshReader()
{}


// ************************************************************************* //