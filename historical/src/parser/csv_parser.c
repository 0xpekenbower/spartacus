#include "csv_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_LINE_LENGTH 1024

// Helper to check if string ends with suffix
static int ends_with(const char *str, const char *suffix) {
    if (!str || !suffix) return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr) return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

// Comparator for qsort
static int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

Candle* parse_csv(const char *filename, size_t *count) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    char line[MAX_LINE_LENGTH];
    size_t capacity = 1000;
    size_t n = 0;
    Candle *candles = malloc(sizeof(Candle) * capacity);

    while (fgets(line, sizeof(line), file)) {
        if (n >= capacity) {
            capacity *= 2;
            candles = realloc(candles, sizeof(Candle) * capacity);
        }

        Candle *c = &candles[n];
        // Initialize default values
        strcpy(c->s, "BTCUSDT"); 
        strcpy(c->i, "1m");      

        // Parse line
        // openTime,open,high,low,close,volume,closeTime,quoteVolume,numberOfTrades,takerBuyVolume,takerBuyQuoteVolume,ignore
        char *token;
        char *rest = line;

        // openTime
        token = strtok_r(rest, ",", &rest); c->t = token ? atof(token) : 0;
        // open
        token = strtok_r(rest, ",", &rest); c->o = token ? atof(token) : 0;
        // high
        token = strtok_r(rest, ",", &rest); c->h = token ? atof(token) : 0;
        // low
        token = strtok_r(rest, ",", &rest); c->l = token ? atof(token) : 0;
        // close
        token = strtok_r(rest, ",", &rest); c->c = token ? atof(token) : 0;
        // volume
        token = strtok_r(rest, ",", &rest); c->v = token ? atof(token) : 0;
        // closeTime
        token = strtok_r(rest, ",", &rest); c->T = token ? atof(token) : 0;
        
        n++;
    }

    fclose(file);
    *count = n;
    return candles;
}

Candle* parse_csv_directory(const char *directory, size_t *count) {
    DIR *d;
    struct dirent *dir;
    d = opendir(directory);
    if (!d) {
        perror("Error opening directory");
        return NULL;
    }

    // Collect CSV files
    char **files = NULL;
    size_t file_count = 0;
    size_t file_cap = 0;

    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_REG && ends_with(dir->d_name, ".csv")) {
            if (file_count >= file_cap) {
                file_cap = (file_cap == 0) ? 16 : file_cap * 2;
                files = realloc(files, sizeof(char*) * file_cap);
            }
            files[file_count++] = strdup(dir->d_name);
        }
    }
    closedir(d);

    if (file_count == 0) {
        if (files) free(files);
        *count = 0;
        return NULL;
    }

    // Sort files
    qsort(files, file_count, sizeof(char*), compare_strings);

    // Read all files
    Candle *all_candles = NULL;
    size_t total_count = 0;
    size_t total_cap = 0;

    for (size_t i = 0; i < file_count; i++) {
        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", directory, files[i]);
        
        size_t c_count = 0;
        Candle *c = parse_csv(filepath, &c_count);
        
        if (c && c_count > 0) {
            if (total_count + c_count > total_cap) {
                total_cap = (total_count + c_count) * 2;
                if (total_cap < 1000) total_cap = 1000;
                all_candles = realloc(all_candles, sizeof(Candle) * total_cap);
            }
            memcpy(all_candles + total_count, c, sizeof(Candle) * c_count);
            total_count += c_count;
            free(c);
        }
        free(files[i]);
    }
    free(files);

    *count = total_count;
    return all_candles;
}