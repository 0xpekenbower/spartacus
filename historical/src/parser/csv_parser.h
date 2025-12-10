#ifndef CSV_PARSER_H
#define CSV_PARSER_H

#include "../../inc/types.h"
#include <stddef.h>

// Reads CSV file and returns array of Candles.
// count is updated with the number of candles read.
Candle* parse_csv(const char *filename, size_t *count);

// Reads all CSV files in a directory (sorted alphabetically) and returns combined array of Candles.
Candle* parse_csv_directory(const char *directory, size_t *count);

#endif
