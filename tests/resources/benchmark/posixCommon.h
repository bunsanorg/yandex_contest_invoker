#include "buffer.h"

#include <unistd.h>

int errRet(int code, const char *cmd) {
  if (code < 0) {
    perror(cmd);
    abort();
  }
  return code;
}

#define ERR(X) errRet(X, #X)

ssize_t readAll() {
  ssize_t readBytes, readBytesInternal;
  readBytes = ERR(read(STDIN_FILENO, buf, BUFSIZE));
  if (!readBytes) return 0;
  while ((readBytes < BUFSIZE) &&
         (readBytesInternal =
              ERR(read(STDIN_FILENO, buf + readBytes, BUFSIZE - readBytes)))) {
    readBytes += readBytesInternal;
  }
  assert(readBytes == BUFSIZE);
  return readBytes;
}

ssize_t writeAll() {
  ssize_t writeBytes, writeBytesInternal;
  writeBytes = ERR(write(STDOUT_FILENO, buf, BUFSIZE));
  while ((writeBytes < BUFSIZE) &&
         (writeBytesInternal = ERR(
              write(STDOUT_FILENO, buf + writeBytes, BUFSIZE - writeBytes)))) {
    writeBytes += writeBytesInternal;
  }
  assert(writeBytes == BUFSIZE);
  return writeBytes;
}
