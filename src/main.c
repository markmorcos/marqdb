#include "sql.h"
#include <stdio.h>

int main() {
  DiskManager* dm = disk_open("test.db");
  BufferPool* bp = bp_create(dm, 32);

  repl(bp);

  bp_destroy(bp);
  disk_close(dm);
  
  return 0;
}
