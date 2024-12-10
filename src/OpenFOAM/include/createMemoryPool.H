//dummy pool
Foam::Info << "create dummyMemoryPool\n " << Foam::endl;

MemoryPool* memPool = Foam::MemoryPool::New("dummyMemoryPool", 0);

//fixed size
//Info << "create fixedSizeMemoryPool\n " << endl;

//MemoryPool* memPool = MemoryPool::New("fixedSizeMemoryPool", 32*1024ul*1024*1024);
