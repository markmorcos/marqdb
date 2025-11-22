#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "disk.h"
#include "page.h"

/**
 * @brief Buffer frame structure representing a single page in the buffer pool
 * 
 * Each BufferFrame holds metadata about the page it contains, including its
 * page ID, validity, dirty status, pin count, and reference bit for replacement
 * policies. The actual page data is stored in the 'page' member.
 */
typedef struct {
  uint32_t page_id; ///< Unique identifier of the page stored in this frame
  bool is_valid; ///< Indicates if the frame contains a valid page
  bool is_dirty; ///< Indicates if the page has been modified
  int pin_count; ///< Number of active pins on the page
  bool refbit; ///< Reference bit used for the clock replacement policy
  Page page; ///< The actual page data stored in this frame
} BufferFrame;

/**
 * @brief Buffer pool structure for managing in-memory pages
 * 
 * The BufferPool structure manages a collection of BufferFrames, providing
 * functionality to fetch, unpin, and flush pages. It uses a clock replacement
 * policy to manage page eviction when the pool reaches its capacity.
 */
typedef struct {
  DiskManager* dm; ///< Associated disk manager for I/O operations
  int capacity; ///< Maximum number of pages in the buffer pool
  BufferFrame* frames; ///< Array of buffer frames
  int clock_hand; ///< Current position of the clock hand for replacement policy
} BufferPool;

/**
 * @brief Creates a new buffer pool with the specified capacity and associated disk manager.
 * 
 * This function initializes a BufferPool instance that manages a fixed number
 * of pages in memory, using the provided DiskManager for disk I/O operations.
 * 
 * @param dm Pointer to the DiskManager for disk I/O operations
 * @param capacity The maximum number of pages the buffer pool can hold
 * @return BufferPool* Pointer to the newly created BufferPool instance
 */
BufferPool* bp_create(DiskManager* dm, int capacity);

/**
 * @brief Destroys the buffer pool, releasing all associated resources.
 * 
 * This function flushes all dirty pages to disk and frees memory allocated
 * for the buffer pool.
 * 
 * @param bp Pointer to the BufferPool to be destroyed
 */
void bp_destroy(BufferPool* bp);

/**
 * @brief Fetches a page from the buffer pool, loading it from disk if necessary.
 * 
 * If the requested page is not already in the buffer pool, this function
 * loads it from disk into a buffer frame, potentially evicting another page
 * based on the clock replacement policy.
 * 
 * @param bp Pointer to the BufferPool instance
 * @param page_id The unique identifier of the page to fetch
 * @return Page* Pointer to the fetched Page structure
 */
Page* bp_fetch_page(BufferPool* bp, uint32_t page_id);

/**
 * @brief Unpins a page in the buffer pool, optionally marking it as dirty.
 * 
 * This function decreases the pin count of the specified page, indicating
 * that it is no longer in use. If the 'dirty' flag is set to true, the page
 * is marked as dirty.
 * 
 * @param bp Pointer to the BufferPool instance
 * @param page_id The unique identifier of the page to unpin
 * @param dirty Boolean flag indicating if the page has been modified
 */
void bp_unpin_page(BufferPool* bp, uint32_t page_id, bool dirty);

/**
 * @brief Flushes all dirty pages in the buffer pool to disk.
 * 
 * This function iterates through all pages in the buffer pool and writes
 * any dirty pages back to disk to ensure data persistence.
 * 
 * @param bp Pointer to the BufferPool instance
 */
void bp_flush_all(BufferPool* bp);
