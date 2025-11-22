#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "disk.h"
#include "page.h"

/**
 * @brief Heap file structure representing a collection of pages storing records.
 * 
 * A HeapFile manages a linked list of data pages, starting from a header page.
 * It provides functionality to insert, retrieve, and scan records stored across
 * multiple pages in the database system.
 */
typedef struct {
  uint32_t header_page_id; ///< Page ID of the heap file's header page
  uint32_t first_data_pid; ///< Page ID of the first data page in the heap file
  uint32_t last_data_pid;  ///< Page ID of the last data page in the heap file
} HeapFile;

/**
 * @brief Record Identifier (RID) structure for uniquely identifying records.
 * 
 * An RID consists of a page ID and a slot ID, allowing precise location of
 * a record within the heap file's pages.
 */
typedef struct {
  uint32_t page_id;   ///< Page ID where the record is stored
  uint16_t slot_id;   ///< Slot ID within the page where the record is stored
} RID;

/**
 * @brief Opens or creates a heap file on the given DiskManager.
 * 
 * If the heap file does not exist, it will be created with an initial header page.
 * 
 * @param dm Pointer to the DiskManager for disk operations
 * @return HeapFile structure representing the opened or created heap file
 */
HeapFile heap_open(DiskManager* dm);

/**
 * @brief Inserts a record into the heap file.
 * 
 * This function finds a suitable page with enough free space to store the record.
 * If no such page exists, a new page is allocated. The record is then inserted,
 * and its RID is returned.
 * 
 * @param dm Pointer to the DiskManager for disk operations
 * @param hf Pointer to the HeapFile where the record will be inserted
 * @param rec Pointer to the record data to be inserted
 * @param len Length of the record in bytes
 * @return RID of the inserted record
 */
RID heap_insert(DiskManager* dm, HeapFile* hf, const uint8_t* rec, uint16_t len);

/**
 * @brief Retrieves a record from the heap file using its RID.
 * 
 * This function locates the page and slot specified by the RID and retrieves
 * the corresponding record data.
 * 
 * @param dm Pointer to the DiskManager for disk operations
 * @param rid RID of the record to retrieve
 * @param out Output parameter that will point to the record data
 * @param len Output parameter that will hold the length of the record
 * @return true if the record was successfully retrieved, false otherwise
 */
bool heap_get(DiskManager* dm, RID rid, uint8_t** out, uint16_t* len);

/**
 * @brief Scans the heap file to retrieve the next record in sequence.
 * 
 * This function uses a cursor (RID) to keep track of the current position
 * in the heap file and retrieves the next available record.
 * 
 * @param dm Pointer to the DiskManager for disk operations
 * @param hf Pointer to the HeapFile being scanned
 * @param cursor Pointer to the RID cursor indicating the current position
 * @param out Output parameter that will point to the record data
 * @param len Output parameter that will hold the length of the record
 * @return true if the next record was successfully retrieved, false otherwise
 */
bool heap_scan_next(DiskManager* dm, HeapFile* hf, RID* cursor, uint8_t** out, uint16_t* len);