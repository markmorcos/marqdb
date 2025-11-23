#pragma once
#include <stdint.h>
#include "catalog.h"

/**
 * @brief Encodes a row of data based on the provided column definitions.
 * 
 * This function takes an array of column definitions and their corresponding
 * string values, encodes them into a binary format, and writes the result
 * into the provided output buffer.
 * 
 * @param cols Array of column definitions
 * @param ncols Number of columns
 * @param values Array of string values corresponding to each column
 * @param nvalues Number of values provided
 * @param out Output buffer to write the encoded row data
 * @param out_cap Capacity of the output buffer
 * @return int The length of the encoded row data, or -1 on error
 */
int row_encode(const ColumnDef* cols, int ncols,
               const char** values, int nvalues,
               uint8_t* out, int out_cap);

/**
 * @brief Decodes a binary row of data into a human-readable string format.
 * 
 * This function takes a binary-encoded row and decodes it based on the
 * provided column definitions, producing a textual representation of the row.
 * 
 * @param cols Array of column definitions
 * @param ncols Number of columns
 * @param row Binary-encoded row data
 * @param row_len Length of the binary row data
 * @param out_text Output buffer to write the decoded text
 * @param out_cap Capacity of the output text buffer
 * @return int The length of the decoded text, or -1 on error
 */
int row_decode(const ColumnDef* cols, int ncols,
               const uint8_t* row, int row_len,
               char* out_text, int out_cap);
