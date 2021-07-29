#ifndef FRYSTL_ASSERT_H
#define FRYSTL_ASSERT_H

#define FRYSTL_ASSERT(assertion) if (!(assertion)) FryStlAssertFailed(#assertion,__FILE__,__LINE__)

#include <cstdio>       // stderr
#include <csignal>      // raise, SIGABRT
static void FryStlAssertFailed(const char* assertion, const char* file, unsigned line)
{
    fprintf(stderr, "Assertion \"%s\" failed in %s at line %u.\n", 
        assertion, file, line);
    raise(SIGABRT);
}

#endif  // ndef FRYSTL_ASSERT_H