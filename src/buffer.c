#include "buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INVALID_PID 0xFFFFFFFF

static int find_frame(BufferPool* bp, uint32_t pid) {
  for (int i = 0; i < bp->capacity; i++) {
    if (bp->frames[i].is_valid && bp->frames[i].page_id == pid) {
      return i;
    }
  }
  return -1;
}

static int pick_victim(BufferPool* bp) {
  int scanned = 0;
  while (scanned < bp->capacity * 2) {
    int i = bp->clock_hand;
    BufferFrame* f = &bp->frames[i];

    if (!f->is_valid) {
      bp->clock_hand = (i + 1) % bp->capacity;
      return i;
    }

    if (f->pin_count == 0) {
      if (f->refbit) {
        f->refbit = false;
      } else {
        bp->clock_hand = (i + 1) % bp->capacity;
        return i;
      }
    }

    bp->clock_hand = (i + 1) % bp->capacity;
    scanned++;
  }

  return -1;
}

BufferPool* bp_create(DiskManager* dm, int capacity) {
  BufferPool* bp = calloc(1, sizeof(*bp));
  bp->dm = dm;
  bp->capacity = capacity;
  bp->frames = calloc(capacity, sizeof(BufferFrame));
  bp->clock_hand = 0;

  for (int i = 0; i < capacity; i++) {
    bp->frames[i].page_id = INVALID_PID;
  }
  return bp;
}

void bp_flush_all(BufferPool* bp) {
  for (int i = 0; i < bp->capacity; i++) {
    BufferFrame* f = &bp->frames[i];
    if (f->is_valid && f->is_dirty) {
      disk_write_page(bp->dm, f->page_id, &f->page);
      f->is_dirty = false;
    }
  }
}

void bp_destroy(BufferPool* bp) {
  if (!bp) return;
  bp_flush_all(bp);
  free(bp->frames);
  free(bp);
}

Page* bp_fetch_page(BufferPool* bp, uint32_t page_id) {
  int idx = find_frame(bp, page_id);
  if (idx >= 0) {
    BufferFrame* f = &bp->frames[idx];
    f->pin_count++;
    f->refbit = true;
    return &f->page;
  }

  int victim = pick_victim(bp);
  if (victim < 0) {
    fprintf(stderr, "BufferPool full: all pages pinned\n");
    return NULL;
  }

  BufferFrame* f = &bp->frames[victim];

  if (f->is_valid && f->is_dirty) {
    disk_write_page(bp->dm, f->page_id, &f->page);
  }

  disk_read_page(bp->dm, page_id, &f->page);

  f->page_id = page_id;
  f->is_valid = true;
  f->is_dirty = false;
  f->pin_count = 1;
  f->refbit = true;

  return &f->page;
}

void bp_unpin_page(BufferPool* bp, uint32_t page_id, bool dirty) {
  int idx = find_frame(bp, page_id);
  if (idx < 0) return;

  BufferFrame* f = &bp->frames[idx];
  if (dirty) f->is_dirty = true;
  if (f->pin_count > 0) f->pin_count--;
}
