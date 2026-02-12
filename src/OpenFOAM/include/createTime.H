std::fprintf(stderr, "TRACE: createTime step 1 - before Time ctor\n");
std::fflush(stderr);
std::fprintf(stderr, "TRACE: controlDictName at %p\n",
    (const void*)&Foam::Time::controlDictName);
std::fflush(stderr);

Foam::Time runTime(Foam::Time::controlDictName, args);

std::fprintf(stderr, "TRACE: createTime step 2 - after Time ctor\n");
std::fflush(stderr);
