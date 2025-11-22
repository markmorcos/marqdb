#pragma once
#include <stdint.h>
#include "buffer.h"

#define CATALOG_PID 0
#define CATALOG_MAGIC "MARQDB1"
#define INVALID_PID 0xFFFFFFFF

/**
 * @brief The Catalog structure holds metadata about the database catalog.
 * 
 * It currently contains the PID of the heap file's header page.
 */
typedef struct {
  uint32_t heap_header_pid;
} Catalog;

/**
  * @brief Opens the catalog from the buffer pool.
  * 
  * This function reads the catalog information from the designated catalog page
  * in the buffer pool and returns a Catalog structure populated with the data.
  * 
  * @param bp Pointer to the BufferPool instance managing memory pages
  * @return Catalog The populated Catalog structure
  */
Catalog catalog_open(BufferPool* bp);

/**
 * @brief Writes the catalog data to the buffer pool.
 * 
 * This function serializes the Catalog structure and writes it to the
 * designated catalog page in the buffer pool for persistent storage.
 * 
 * @param bp Pointer to the BufferPool instance managing memory pages
 * @param c Pointer to the Catalog structure containing catalog data to write
 */
void catalog_write(BufferPool* bp, const Catalog* c);
