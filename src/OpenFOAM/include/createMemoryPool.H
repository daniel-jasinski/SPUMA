// create memory pool from arglist options

const word pool = args.getOrDefault<word>("pool","dummyMemoryPool");
const scalar poolSize = args.getOrDefault<scalar>("poolsize",0);

Info << "create " << pool <<"\n " << endl;
MemoryPool* memPool = MemoryPool::New(pool, poolSize*1024*1024*1024ul);

