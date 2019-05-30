/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2018-2019 OpenCFD Ltd.
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

#include "IOobjectList.H"
#include "predicates.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

// Templated implementation for classes()
template<class MatchPredicate>
Foam::HashTable<Foam::wordHashSet> Foam::IOobjectList::classesImpl
(
    const IOobjectList& list,
    const MatchPredicate& matchName
)
{
    HashTable<wordHashSet> summary(2*list.size());

    // Summary (key,val) = (class-name, object-names)
    forAllConstIters(list, iter)
    {
        const word& key = iter.key();
        const IOobject* io = iter.object();

        if (matchName(key))
        {
            // Create entry (if needed) and insert
            summary(io->headerClassName()).insert(key);
        }
    }

    return summary;
}


// Templated implementation for count()
template<class MatchPredicate1, class MatchPredicate2>
Foam::label Foam::IOobjectList::countImpl
(
    const IOobjectList& list,
    const MatchPredicate1& matchClass,
    const MatchPredicate2& matchName
)
{
    label count = 0;

    forAllConstIters(list, iter)
    {
        const IOobject* io = iter.object();

        if (matchClass(io->headerClassName()) && matchName(io->name()))
        {
            ++count;
        }
    }

    return count;
}


// Templated implementation for count()
template<class Type, class MatchPredicate>
Foam::label Foam::IOobjectList::countTypeImpl
(
    const IOobjectList& list,
    const MatchPredicate& matchName
)
{
    label count = 0;

    forAllConstIters(list, iter)
    {
        const IOobject* io = iter.object();

        if (io->isHeaderClassName<Type>() && matchName(io->name()))
        {
            ++count;
        }
    }

    return count;
}


// Templated implementation for names(), sortedNames()
template<class MatchPredicate1, class MatchPredicate2>
Foam::wordList Foam::IOobjectList::namesImpl
(
    const IOobjectList& list,
    const MatchPredicate1& matchClass,
    const MatchPredicate2& matchName,
    const bool doSort
)
{
    wordList objNames(list.size());

    label count = 0;
    forAllConstIters(list, iter)
    {
        const word& key = iter.key();
        const IOobject* io = iter.object();

        if (matchClass(io->headerClassName()) && matchName(key))
        {
            objNames[count] = key;
            ++count;
        }
    }

    objNames.resize(count);

    if (doSort)
    {
        Foam::sort(objNames);
    }

    return objNames;
}


// Templated implementation for names(), sortedNames()
template<class Type, class MatchPredicate>
Foam::wordList Foam::IOobjectList::namesTypeImpl
(
    const IOobjectList& list,
    const MatchPredicate& matchName,
    const bool doSort
)
{
    wordList objNames(list.size());

    label count = 0;
    forAllConstIters(list, iter)
    {
        const word& key = iter.key();
        const IOobject* io = iter.object();

        if (io->isHeaderClassName<Type>() && matchName(key))
        {
            objNames[count] = key;
            ++count;
        }
    }

    objNames.resize(count);

    if (doSort)
    {
        Foam::sort(objNames);
    }

    return objNames;
}


// Templated implementation for lookup()
template<class MatchPredicate>
Foam::IOobjectList Foam::IOobjectList::lookupImpl
(
    const IOobjectList& list,
    const MatchPredicate& matchName
)
{
    IOobjectList results(list.size());

    forAllConstIters(list, iter)
    {
        const word& key = iter.key();
        const IOobject* io = iter.object();

        if (matchName(key))
        {
            if (IOobject::debug)
            {
                InfoInFunction << "Found " << key << endl;
            }

            results.set(key, new IOobject(*io));
        }
    }

    return results;
}


// Templated implementation for lookupClass()
template<class MatchPredicate1, class MatchPredicate2>
Foam::IOobjectList Foam::IOobjectList::lookupClassImpl
(
    const IOobjectList& list,
    const MatchPredicate1& matchClass,
    const MatchPredicate2& matchName
)
{
    IOobjectList results(list.size());

    forAllConstIters(list, iter)
    {
        const word& key = iter.key();
        const IOobject* io = iter.object();

        if (matchClass(io->headerClassName()) && matchName(key))
        {
            if (IOobject::debug)
            {
                InfoInFunction << "Found " << key << endl;
            }

            results.set(key, new IOobject(*io));
        }
    }

    return results;
}


// Templated implementation for lookupClass()
template<class Type, class MatchPredicate>
Foam::IOobjectList Foam::IOobjectList::lookupClassTypeImpl
(
    const IOobjectList& list,
    const MatchPredicate& matchName
)
{
    IOobjectList results(list.size());

    forAllConstIters(list, iter)
    {
        const word& key = iter.key();
        const IOobject* io = iter.object();

        if (io->isHeaderClassName<Type>() && matchName(key))
        {
            if (IOobject::debug)
            {
                InfoInFunction << "Found " << key << endl;
            }

            results.set(key, new IOobject(*io));
        }
    }

    return results;
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
const Foam::IOobject* Foam::IOobjectList::cfindObject
(
    const word& objName
) const
{
    const_iterator iter = cfind(objName);

    if (iter.found())
    {
        const IOobject* io = iter.object();

        if (io->isHeaderClassName<Type>())
        {
            if (IOobject::debug)
            {
                InfoInFunction << "Found " << objName << endl;
            }

            return io;
        }
        else if (IOobject::debug)
        {
            InfoInFunction
                << "Found " << objName << " of different type" << endl;
        }
    }
    else if (IOobject::debug)
    {
        InfoInFunction << "Could not find " << objName << endl;
    }

    return nullptr;
}


template<class Type>
const Foam::IOobject* Foam::IOobjectList::findObject
(
    const word& objName
) const
{
    return cfindObject<Type>(objName);
}


template<class Type>
Foam::IOobject* Foam::IOobjectList::findObject(const word& objName)
{
    return const_cast<IOobject*>(cfindObject<Type>(objName));
}


template<class Type>
Foam::IOobject* Foam::IOobjectList::getObject(const word& objName) const
{
    return const_cast<IOobject*>(cfindObject<Type>(objName));
}


template<class MatchPredicate>
Foam::IOobjectList Foam::IOobjectList::lookup
(
    const MatchPredicate& matchName
) const
{
    return lookupImpl(*this, matchName);
}


template<class MatchPredicate>
Foam::IOobjectList Foam::IOobjectList::lookupClass
(
    const MatchPredicate& matchClass
) const
{
    return lookupClassImpl(*this, matchClass, predicates::always());
}


template<class MatchPredicate1, class MatchPredicate2>
Foam::IOobjectList Foam::IOobjectList::lookupClass
(
    const MatchPredicate1& matchClass,
    const MatchPredicate2& matchName
) const
{
    return lookupClassImpl(*this, matchClass, matchName);
}


template<class Type>
Foam::IOobjectList Foam::IOobjectList::lookupClass() const
{
    return lookupClassTypeImpl<Type>(*this, predicates::always());
}


template<class Type, class MatchPredicate>
Foam::IOobjectList Foam::IOobjectList::lookupClass
(
    const MatchPredicate& matchName
) const
{
    return lookupClassImpl<Type>(*this, matchName);
}


template<class MatchPredicate>
Foam::HashTable<Foam::wordHashSet>
Foam::IOobjectList::classes
(
    const MatchPredicate& matchName
) const
{
    return classesImpl(*this, matchName);
}


template<class MatchPredicate>
Foam::label Foam::IOobjectList::count
(
    const MatchPredicate& matchClass
) const
{
    return countImpl(*this, matchClass, predicates::always());
}


template<class MatchPredicate1, class MatchPredicate2>
Foam::label Foam::IOobjectList::count
(
    const MatchPredicate1& matchClass,
    const MatchPredicate2& matchName
) const
{
    return countImpl(*this, matchClass, matchName);
}


template<class Type>
Foam::label Foam::IOobjectList::count() const
{
    return countTypeImpl<Type>(*this, predicates::always());
}


template<class Type, class MatchPredicate>
Foam::label Foam::IOobjectList::count
(
    const MatchPredicate& matchName
) const
{
    return countTypeImpl<Type>(*this, matchName);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class MatchPredicate>
Foam::wordList Foam::IOobjectList::names
(
    const MatchPredicate& matchClass
) const
{
    return namesImpl(*this, matchClass, predicates::always(), false);
}


template<class MatchPredicate>
Foam::wordList Foam::IOobjectList::names
(
    const MatchPredicate& matchClass,
    const bool syncPar
) const
{
    wordList objNames
    (
        namesImpl(*this, matchClass, predicates::always(), false)
    );

    checkNames(objNames, syncPar);
    return objNames;
}


template<class MatchPredicate1, class MatchPredicate2>
Foam::wordList Foam::IOobjectList::names
(
    const MatchPredicate1& matchClass,
    const MatchPredicate2& matchName
) const
{
    return namesImpl(*this, matchClass, matchName, false);
}


template<class MatchPredicate1, class MatchPredicate2>
Foam::wordList Foam::IOobjectList::names
(
    const MatchPredicate1& matchClass,
    const MatchPredicate2& matchName,
    const bool syncPar
) const
{
    wordList objNames(namesImpl(*this, matchClass, matchName, false));

    checkNames(objNames, syncPar);
    return objNames;
}


template<class Type>
Foam::wordList Foam::IOobjectList::names() const
{
    return namesTypeImpl<Type>(*this, predicates::always(), false);
}


template<class Type>
Foam::wordList Foam::IOobjectList::names(const bool syncPar) const
{
    wordList objNames(namesTypeImpl<Type>(*this, predicates::always(), false));

    checkNames(objNames, syncPar);
    return objNames;
}


template<class Type, class MatchPredicate>
Foam::wordList Foam::IOobjectList::names
(
    const MatchPredicate& matchName
) const
{
    return namesTypeImpl<Type>(*this, matchName, false);
}


template<class Type, class MatchPredicate>
Foam::wordList Foam::IOobjectList::names
(
    const MatchPredicate& matchName,
    const bool syncPar
) const
{
    wordList objNames(namesTypeImpl<Type>(*this, matchName, false));

    checkNames(objNames, syncPar);
    return objNames;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class MatchPredicate>
Foam::wordList Foam::IOobjectList::sortedNames
(
    const MatchPredicate& matchClass
) const
{
    return namesImpl(*this, matchClass, predicates::always(), true);
}


template<class MatchPredicate>
Foam::wordList Foam::IOobjectList::sortedNames
(
    const MatchPredicate& matchClass,
    const bool syncPar
) const
{
    wordList objNames
    (
        namesImpl(*this, matchClass, predicates::always(), true)
    );

    checkNames(objNames, syncPar);
    return objNames;
}


template<class MatchPredicate1, class MatchPredicate2>
Foam::wordList Foam::IOobjectList::sortedNames
(
    const MatchPredicate1& matchClass,
    const MatchPredicate2& matchName
) const
{
    return namesImpl(*this, matchClass, matchName, true);
}

template<class MatchPredicate1, class MatchPredicate2>
Foam::wordList Foam::IOobjectList::sortedNames
(
    const MatchPredicate1& matchClass,
    const MatchPredicate2& matchName,
    const bool syncPar
) const
{
    wordList objNames(namesImpl(*this, matchClass, matchName, true));

    checkNames(objNames, syncPar);
    return objNames;
}


template<class Type>
Foam::wordList Foam::IOobjectList::sortedNames() const
{
    return namesTypeImpl<Type>(*this, predicates::always(), true);
}


template<class Type>
Foam::wordList Foam::IOobjectList::sortedNames(const bool syncPar) const
{
    wordList objNames(namesTypeImpl<Type>(*this, predicates::always(), true));

    checkNames(objNames, syncPar);
    return objNames;
}


template<class Type, class MatchPredicate>
Foam::wordList Foam::IOobjectList::sortedNames
(
    const MatchPredicate& matchName
) const
{
    return namesTypeImpl<Type>(*this, matchName, true);
}


template<class Type, class MatchPredicate>
Foam::wordList Foam::IOobjectList::sortedNames
(
    const MatchPredicate& matchName,
    const bool syncPar
) const
{
    return namesTypeImpl<Type>(*this, matchName, true, syncPar);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class UnaryPredicate>
Foam::label Foam::IOobjectList::filterClasses
(
    const UnaryPredicate& pred,
    const bool pruning
)
{
// This is like
// return HashPtrTable<IOobject>::filterValues
// (
//     [&](const IOobject* io){ return pred(io->headerClassName()); },
//     pruning
// );
// which is really
// return HashTable<IOobject*>::filterValues
//
// except that it does not leak

    label changed = 0;

    for (iterator iter = begin(); iter != end(); ++iter)
    {
        // Matches? either prune (pruning) or keep (!pruning)
        if
        (
            (pred(iter.object()->headerClassName()) ? pruning : !pruning)
         && erase(iter)
        )
        {
            ++changed;
        }
    }

    return changed;
}


template<class UnaryPredicate>
Foam::label Foam::IOobjectList::filterObjects
(
    const UnaryPredicate& pred,
    const bool pruning
)
{
// This is like
// return HashPtrTable<IOobject>::filterKeys(pred, pruning);
// which is really
// return HashTable<IOobject*>::filterKeys(pred, pruning);
//
// except that it does not leak

    label changed = 0;

    for (iterator iter = begin(); iter != end(); ++iter)
    {
        // Matches? either prune (pruning) or keep (!pruning)
        if
        (
            (pred(iter.key()) ? pruning : !pruning)
         && erase(iter)
        )
        {
            ++changed;
        }
    }

    return changed;
}


template<class Type>
Foam::wordList Foam::IOobjectList::allNames() const
{
    wordList objNames(namesTypeImpl<Type>(*this, predicates::always(), false));

    syncNames(objNames);
    return objNames;
}


// ************************************************************************* //