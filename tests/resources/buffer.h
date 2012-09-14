#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define BUFSIZE 20

char srcBuf[BUFSIZE];

char buf[BUFSIZE];

void clearAll()
{
    memset(buf, 0, BUFSIZE);
    // make invalid
    buf[BUFSIZE - 1] = 1;
}

void fillAll()
{
    unsigned char c = 0;
    for (unsigned i = 0; i + 1 < BUFSIZE; ++i)
        c ^= (srcBuf[i] = buf[i] = 'a' + (rand() % 50));
    srcBuf[BUFSIZE - 1] = buf[BUFSIZE - 1] = c;
}

void fastValidateAll()
{
    unsigned char c = 0;
    for (unsigned i = 0; i < BUFSIZE; ++i)
    {
        if (i + 1 < BUFSIZE)
            assert(buf[i] != 0);
        c ^= buf[i];
    }
    assert(c == 0);
}

void validateAll()
{
    for (unsigned i = 0; i < BUFSIZE; ++i)
        if (srcBuf[i] != buf[i])
        {
            fprintf(stderr, "srcBuf[%u] != buf[%u]: %d != %d\n", i, i, (int)srcBuf[i], (int)buf[i]);
            abort();
        }
}
