#include "row.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static void write_u16(uint8_t* p, uint16_t v) {
  p[0] = (uint8_t)(v & 0xFF);
  p[1] = (uint8_t)(v >> 8);
}
static uint16_t read_u16(const uint8_t* p) {
  return (uint16_t)(p[0] | (p[1] << 8));
}

int row_encode(const ColumnDef* cols, int ncols,
               const char** values, int nvalues,
               uint8_t* out, int out_cap) {
  if (nvalues != ncols) return -1;

  int pos = 0;
  if (out_cap < 2) return -1;
  write_u16(out + pos, (uint16_t)ncols);
  pos += 2;

  for (int i = 0; i < ncols; i++) {
    if (cols[i].type == COL_INT) {
      long v = strtol(values[i], NULL, 10);
      if (pos + 4 > out_cap) return -1;
      out[pos+0] = (uint8_t)(v & 0xFF);
      out[pos+1] = (uint8_t)((v >> 8) & 0xFF);
      out[pos+2] = (uint8_t)((v >> 16) & 0xFF);
      out[pos+3] = (uint8_t)((v >> 24) & 0xFF);
      pos += 4;
    } else if (cols[i].type == COL_TEXT) {
      uint16_t L = (uint16_t)strlen(values[i]);
      if (pos + 2 + L > out_cap) return -1;
      write_u16(out + pos, L);
      pos += 2;
      memcpy(out + pos, values[i], L);
      pos += L;
    } else {
      return -1;
    }
  }

  return pos;
}

int row_decode(const ColumnDef* cols, int ncols,
               const uint8_t* row, int row_len,
               char* out_text, int out_cap) {
  int pos = 0;
  if (row_len < 2) return -1;

  uint16_t stored = read_u16(row);
  pos += 2;
  if (stored != ncols) return -1;

  int written = 0;
  for (int i = 0; i < ncols; i++) {
    if (written + 4 >= out_cap) return -1;

    if (cols[i].type == COL_INT) {
      if (pos + 4 > row_len) return -1;
      int32_t v =
        (int32_t)(row[pos] |
                 (row[pos+1] << 8) |
                 (row[pos+2] << 16) |
                 (row[pos+3] << 24));
      pos += 4;
      written += snprintf(out_text + written, out_cap - written,
                          "%s=%d%s",
                          cols[i].col, v, (i==ncols-1?"":" | "));
    } else if (cols[i].type == COL_TEXT) {
      if (pos + 2 > row_len) return -1;
      uint16_t L = read_u16(row + pos);
      pos += 2;
      if (pos + L > row_len) return -1;

      char buf[256];
      int cpy = (L < sizeof(buf)-1)? L : (int)sizeof(buf)-1;
      memcpy(buf, row + pos, cpy);
      buf[cpy] = 0;
      pos += L;

      written += snprintf(out_text + written, out_cap - written,
                          "%s=%s%s",
                          cols[i].col, buf, (i==ncols-1?"":" | "));
    }
  }

  return written;
}
