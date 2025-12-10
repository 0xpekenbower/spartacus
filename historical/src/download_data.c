#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <curl/curl.h>
#include <zip.h>

#define MAX_CONCURRENT_DOWNLOADS 10

typedef struct {
    char url[512];
    char filepath[512];
    char dest_dir[512];
    FILE *fp;
    CURL *easy;
} DownloadTask;

// Helper to create directory recursively
int mkdir_p(const char *path) {
    char tmp[256];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(tmp, S_IRWXU) != 0 && errno != EEXIST) {
                return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(tmp, S_IRWXU) != 0 && errno != EEXIST) {
        return -1;
    }
    return 0;
}

// Parse date string "YYYY-MM-DD" into struct tm
int parse_date(const char *str, struct tm *t) {
    if (sscanf(str, "%d-%d-%d", &t->tm_year, &t->tm_mon, &t->tm_mday) != 3) {
        return -1;
    }
    t->tm_year -= 1900; // Years since 1900
    t->tm_mon -= 1;     // Months 0-11
    t->tm_hour = 0;
    t->tm_min = 0;
    t->tm_sec = 0;
    t->tm_isdst = -1;
    return 0;
}

// Format struct tm to "YYYY-MM-DD"
void format_date(const struct tm *t, char *buf, size_t size) {
    strftime(buf, size, "%Y-%m-%d", t);
}

// Add days to date
void add_days(struct tm *t, int days) {
    t->tm_mday += days;
    mktime(t); // Normalize
}

// Compare two dates
int compare_dates(struct tm *t1, struct tm *t2) {
    char s1[16], s2[16];
    format_date(t1, s1, sizeof(s1));
    format_date(t2, s2, sizeof(s2));
    return strcmp(s1, s2);
}

char** split_string(const char* str, const char* delimiter, int* count) {
    char* copy = strdup(str);
    int cap = 10;
    char** result = malloc(cap * sizeof(char*));
    *count = 0;
    
    char* token = strtok(copy, delimiter);
    while (token) {
        if (*count >= cap) {
            cap *= 2;
            result = realloc(result, cap * sizeof(char*));
        }
        result[(*count)++] = strdup(token);
        token = strtok(NULL, delimiter);
    }
    free(copy);
    return result;
}

void free_string_array(char** arr, int count) {
    for (int i = 0; i < count; i++) free(arr[i]);
    free(arr);
}

int is_valid_timeframe(const char* tf) {
    const char* valid_tfs[] = {
        "12h", "15m", "1d", "1h", "1m", "1s", 
        "2h", "30m", "3m", "4h", "5m", "6h", "8h", NULL
    };
    for (int i = 0; valid_tfs[i]; i++) {
        if (strcmp(tf, valid_tfs[i]) == 0) return 1;
    }
    return 0;
}

void print_progress(size_t current, size_t total, const char *prefix) {
    float percentage = (float)current / total * 100.0;
    int bar_width = 50;
    int pos = bar_width * current / total;

    printf("\r%s [", prefix);
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) printf("=");
        else if (i == pos) printf(">");
        else printf(" ");
    }
    printf("] %.1f%% (%zu/%zu)", percentage, current, total);
    fflush(stdout);
    if (current == total) printf("\n");
}

int extract_zip(const char *zip_path, const char *dest_dir) {
    int err = 0;
    struct zip *z = zip_open(zip_path, 0, &err);
    if (!z) {
        // fprintf(stderr, "Failed to open zip %s: error %d\n", zip_path, err);
        return -1;
    }

    zip_int64_t num_entries = zip_get_num_entries(z, 0);
    for (zip_int64_t i = 0; i < num_entries; i++) {
        const char *name = zip_get_name(z, i, 0);
        if (!name) continue;

        char out_path[1024];
        snprintf(out_path, sizeof(out_path), "%s/%s", dest_dir, name);

        struct zip_stat st;
        if (zip_stat_index(z, i, 0, &st) == 0) {
            struct zip_file *zf = zip_fopen_index(z, i, 0);
            if (!zf) continue;

            FILE *f = fopen(out_path, "wb");
            if (f) {
                char buf[8192];
                zip_int64_t n;
                while ((n = zip_fread(zf, buf, sizeof(buf))) > 0) {
                    fwrite(buf, 1, n, f);
                }
                fclose(f);
            }
            zip_fclose(zf);
        }
    }
    zip_close(z);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: %s <pairs_comma_separated> <timeframes_comma_separated> <start_date:end_date>\n", argv[0]);
        return 1;
    }

    char *pairs_arg = argv[1];
    char *timeframes_arg = argv[2];
    char *dates_arg = argv[3];

    char start_str[16], end_str[16];
    if (sscanf(dates_arg, "%15[^:]:%15s", start_str, end_str) != 2) {
        fprintf(stderr, "Invalid date range format.\n");
        return 1;
    }

    struct tm start_tm = {0}, end_tm = {0};
    if (parse_date(start_str, &start_tm) != 0 || parse_date(end_str, &end_tm) != 0) {
        fprintf(stderr, "Invalid date format.\n");
        return 1;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    CURLM *multi_handle = curl_multi_init();
    curl_multi_setopt(multi_handle, CURLMOPT_MAX_TOTAL_CONNECTIONS, MAX_CONCURRENT_DOWNLOADS);

    // Collect all tasks
    DownloadTask *tasks = NULL;
    size_t task_count = 0;
    size_t task_capacity = 0;

    int pair_count = 0;
    char** pairs = split_string(pairs_arg, ",", &pair_count);
    
    int tf_count = 0;
    char** timeframes = split_string(timeframes_arg, ",", &tf_count);

    for (int i = 0; i < pair_count; i++) {
        for (int j = 0; j < tf_count; j++) {
            char *pair = pairs[i];
            char *tf = timeframes[j];
            
            if (!is_valid_timeframe(tf)) {
                fprintf(stderr, "Warning: Invalid timeframe '%s', skipping.\n", tf);
                continue;
            }

            struct tm current_tm = start_tm;
            while (compare_dates(&current_tm, &end_tm) <= 0) {
                if (task_count >= task_capacity) {
                    task_capacity = (task_capacity == 0) ? 16 : task_capacity * 2;
                    tasks = realloc(tasks, sizeof(DownloadTask) * task_capacity);
                }

                char date_str[16];
                format_date(&current_tm, date_str, sizeof(date_str));

                DownloadTask *t = &tasks[task_count];
                snprintf(t->url, sizeof(t->url), 
                    "https://data.binance.vision/data/spot/daily/klines/%s/%s/%s-%s-%s.zip",
                    pair, tf, pair, tf, date_str);
                
                snprintf(t->dest_dir, sizeof(t->dest_dir), "data/%s/%s", tf, pair);
                mkdir_p(t->dest_dir);
                
                snprintf(t->filepath, sizeof(t->filepath), "%s/%s-%s-%s.zip", t->dest_dir, pair, tf, date_str);
                
                t->fp = fopen(t->filepath, "wb");
                if (!t->fp) {
                    perror("Failed to open file for writing");
                    // Skip incrementing task_count
                } else {
                    t->easy = curl_easy_init();
                    curl_easy_setopt(t->easy, CURLOPT_URL, t->url);
                    curl_easy_setopt(t->easy, CURLOPT_WRITEDATA, t->fp);
                    curl_easy_setopt(t->easy, CURLOPT_FOLLOWLOCATION, 1L);
                    curl_multi_add_handle(multi_handle, t->easy);
                    task_count++;
                }

                add_days(&current_tm, 1);
            }
        }
    }
    
    free_string_array(pairs, pair_count);
    free_string_array(timeframes, tf_count);

    printf("Starting download of %zu files...\n", task_count);

    int still_running = 0;
    int msgs_left = 0;
    CURLMsg *msg;
    size_t completed_downloads = 0;
    print_progress(0, task_count, "Downloading");

    do {
        curl_multi_perform(multi_handle, &still_running);
        
        while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
            if (msg->msg == CURLMSG_DONE) {
                completed_downloads++;
                print_progress(completed_downloads, task_count, "Downloading");
            }
        }

        if(still_running) {
            int numfds;
            curl_multi_wait(multi_handle, NULL, 0, 1000, &numfds);
        }
    } while(still_running);

    printf("Downloads finished. Starting extraction...\n");
    print_progress(0, task_count, "Extracting ");

    for (size_t i = 0; i < task_count; i++) {
        DownloadTask *t = &tasks[i];
        curl_multi_remove_handle(multi_handle, t->easy);
        curl_easy_cleanup(t->easy);
        fclose(t->fp);

        // Check if file is valid (size > 0)
        struct stat st;
        if (stat(t->filepath, &st) == 0 && st.st_size > 0) {
            // printf("Extracting %s...\n", t->filepath);
            if (extract_zip(t->filepath, t->dest_dir) == 0) {
                remove(t->filepath);
            } else {
                // fprintf(stderr, "Failed to extract %s\n", t->filepath);
                remove(t->filepath); // Remove invalid zip
            }
        } else {
            // fprintf(stderr, "File %s is empty or missing, removing.\n", t->filepath);
            remove(t->filepath);
        }
        print_progress(i + 1, task_count, "Extracting ");
    }

    free(tasks);
    curl_multi_cleanup(multi_handle);
    curl_global_cleanup();

    printf("Done.\n");
    return 0;
}
