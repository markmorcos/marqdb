#include "disk.h"
#include "buffer.h"
#include <stdio.h>

void repl(BufferPool* bp);

int main() {
  DiskManager* dm = disk_open("test.db");
  BufferPool* bp  = bp_create(dm, 32);

  repl(bp);

  bp_destroy(bp);
  disk_close(dm);
  return 0;
}
