#ifndef addMemoryPoolOptions_H
#define addMemoryPoolOptions_H
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// Declare memory pool options

argList::addOption
(
    "pool",
    "pool-type",
    "Choose memory pool type"
);

argList::addOption
(
    "poolsize",
    "size [GB]",
    "MemoryPool size (useless for dummyMemoryPool)"
);

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
#endif

// ************************************************************************* //