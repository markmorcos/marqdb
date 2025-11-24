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

/**
 * @brief Represents a decoded value from a row.
 * 
 * This structure holds the type of the column, a flag indicating if the value
 * is NULL, and the actual value which can be either an integer or a text string.
 */
typedef struct {
  ColumnType type; ///< Data type of the column
  int is_null; ///< Flag indicating if the value is NULL
  int32_t i32; ///< Integer value (if type is COL_INT)
  const char* text; ///< Text value (if type is COL_TEXT)
} DecodedValue;

/**
 * @brief Decodes a binary row of data into an array of DecodedValue structures.
 * 
 * This function takes a binary-encoded row and decodes it based on the
 * provided column definitions, populating an array of DecodedValue structures
 * with the decoded values.
 * 
 * @param cols Array of column definitions
 * @param ncols Number of columns
 * @param row Binary-encoded row data
 * @param row_len Length of the binary row data
 * @param out_vals Output array to write the decoded values
 * @param text_scratch Scratch buffer for storing text values
 * @param scratch_cap Capacity of the scratch buffer
 * @return int The number of decoded values, or -1 on error
 */
int row_decode_values(const ColumnDef* cols, int ncols,
                      const uint8_t* row, int row_len,
                      DecodedValue* out_vals,
                      char* text_scratch, int scratch_cap);
