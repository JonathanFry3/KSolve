#ifndef FRYSTL_ASSERT_H
#define FRYSTL_ASSERT_H

#ifdef FRYSTL_DEBUG
#define FRYSTL_ASSERT(assertion) if (!(assertion)) FryStlAssertFailed(#assertion,__FILE__,__LINE__)

#include <cstdio>       // stderr
#include <csignal>      // raise, SIGABRT

static void FryStlAssertFailed(const char* assertion, const char* file, unsigned line)
{
    fprintf(stderr, "Assertion \"%s\" failed in %s at line %u.\n", 
        assertion, file, line);
    raise(SIGABRT);
}

#else       // FRYSTL_DEBUG
#define FRYSTL_ASSERT(assertion)    // empty
#endif      // FRYSTL_DEBUG

#endif  // ndef FRYSTL_ASSERT_H