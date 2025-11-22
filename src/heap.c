#include "heap.h"
#include <string.h>
#include <stdio.h>

#define INVALID_PID 0xFFFFFFFF

static void write_header(BufferPool* bp, HeapFile* hf) {
  Page* p = bp_fetch_page(bp, hf->header_page_id);

  memcpy(p->data + 0, &hf->first_data_pid, sizeof(uint32_t));
  memcpy(p->data + 4, &hf->last_data_pid,  sizeof(uint32_t));

  bp_unpin_page(bp, hf->header_page_id, true);
}

void heap_bootstrap(BufferPool* bp, uint32_t header_pid, uint32_t first_data_pid) {
  HeapFile hf = {
    .header_page_id = header_pid,
    .first_data_pid = first_data_pid,
    .last_data_pid  = first_data_pid
  };
  write_header(bp, &hf);
}

HeapFile heap_open(BufferPool* bp, uint32_t header_pid) {
  HeapFile hf = {0};
  hf.header_page_id = header_pid;

  long size = disk_file_size(bp->dm);

  if (size == 0) {
    uint32_t header_pid = disk_alloc_page(bp->dm);
    uint32_t data_pid = disk_alloc_page(bp->dm);

    hf.header_page_id = header_pid;
    hf.first_data_pid = data_pid;
    hf.last_data_pid = data_pid;

    write_header(bp, &hf);
    return hf;
  }

  Page* p = bp_fetch_page(bp, hf.header_page_id);
  memcpy(&hf.first_data_pid, p->data + 0, sizeof(uint32_t));
  memcpy(&hf.last_data_pid, p->data + 4, sizeof(uint32_t));
  bp_unpin_page(bp, hf.header_page_id, false);

  if (hf.first_data_pid == 0 && hf.last_data_pid == 0) {
    uint32_t data_pid = disk_alloc_page(bp->dm);
    hf.first_data_pid = data_pid;
    hf.last_data_pid = data_pid;
    write_header(bp, &hf);
  }

  return hf;
}

RID heap_insert(BufferPool* bp, HeapFile* hf, const uint8_t* rec, uint16_t len) {
  uint32_t pid = hf->last_data_pid;

  while (1) {
    Page* p = bp_fetch_page(bp, pid);

    int slot = page_insert(p, rec, len);
    if (slot >= 0) {
      bp_unpin_page(bp, pid, true);
      return (RID){ .page_id = pid, .slot_id = (uint16_t)slot };
    }

    if (p->hdr.next_page_id != INVALID_PID) {
      uint32_t next = p->hdr.next_page_id;
      bp_unpin_page(bp, pid, false);
      pid = next;
      continue;
    }

    uint32_t new_pid = disk_alloc_page(bp->dm);
    p->hdr.next_page_id = new_pid;
    bp_unpin_page(bp, pid, true);

    hf->last_data_pid = new_pid;
    write_header(bp, hf);

    pid = new_pid;
  }
}

bool heap_get(BufferPool* bp, RID rid, uint8_t** out, uint16_t* len) {
  Page* p = bp_fetch_page(bp, rid.page_id);
  bool ok = page_get(p, rid.slot_id, out, len);
  bp_unpin_page(bp, rid.page_id, false);
  return ok;
}

bool heap_scan_next(BufferPool* bp, HeapFile* hf, RID* cursor, uint8_t** out, uint16_t* len) {
  uint32_t pid;
  uint16_t slot;

  if (cursor->page_id == INVALID_PID) {
    pid = hf->first_data_pid;
    slot = 0;
  } else {
    pid = cursor->page_id;
    slot = cursor->slot_id + 1;
  }

  while (pid != INVALID_PID) {
    Page* p = bp_fetch_page(bp, pid);

    for (; slot < p->hdr.slot_count; slot++) {
      if (page_get(p, slot, out, len)) {
        cursor->page_id = pid;
        cursor->slot_id = slot;
        return true;
      }
    }

    uint32_t next = p->hdr.next_page_id;
    bp_unpin_page(bp, pid, false);
    pid = next;
    slot = 0;
  }

  return false;
}
