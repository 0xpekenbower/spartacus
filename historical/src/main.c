#include "parser/csv_parser.h"
#include "calc/indicators.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

void ensure_directory_exists(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0700) == -1) {
            if (errno != EEXIST) {
                perror("Error creating directory");
            }
        }
    }
}

void write_csv_header(FILE *f) {
    fprintf(f, "openTime,open,high,low,close,volume,closeTime,");
    fprintf(f, "ema_14,sma_14,wma_14,kama_14,tema_14,trima_14,sar,rsi_14,");
    fprintf(f, "macd,macd_signal,macd_hist,adx_14,adxr_14,apo,aroon_down,aroon_up,aroonosc,");
    fprintf(f, "bop,cci_14,cmo_14,dx_14,mfi_14,minus_di,minus_dm,mom_10,plus_di,plus_dm,ppo,roc_10,");
    fprintf(f, "stoch_k,stoch_d,stochf_k,stochf_d,stochrsi_k,stochrsi_d,trix,ultosc,willr_14,");
    fprintf(f, "obv,ad,adosc,bbands_upper,bbands_middle,bbands_lower,atr_14,natr_14,trange,");
    fprintf(f, "ht_dcperiod,ht_dcphase,ht_phasor_inphase,ht_phasor_quadrature,ht_sine,ht_leadsine,ht_trendmode,");
    fprintf(f, "cdl_doji,cdl_hammer,cdl_engulfing,cdl_morningstar,cdl_eveningstar,cdl_3blackcrows,cdl_3whitesoldiers\n");
}

void write_csv_row(FILE *f, Candle *c, ComputedIndicators *ci, size_t i) {
    fprintf(f, "%.0f,%.8f,%.8f,%.8f,%.8f,%.8f,%.0f,", c->t, c->o, c->h, c->l, c->c, c->v, c->T);
    fprintf(f, "%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,", ci->ema_14[i], ci->sma_14[i], ci->wma_14[i], ci->kama_14[i], ci->tema_14[i], ci->trima_14[i], ci->sar[i], ci->rsi_14[i]);
    fprintf(f, "%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,", ci->macd.macd[i], ci->macd.signal[i], ci->macd.hist[i], ci->adx_14[i], ci->adxr_14[i], ci->apo[i], ci->aroon.aroon_down[i], ci->aroon.aroon_up[i], ci->aroonosc[i]);
    fprintf(f, "%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,", ci->bop[i], ci->cci_14[i], ci->cmo_14[i], ci->dx_14[i], ci->mfi_14[i], ci->minus_di[i], ci->minus_dm[i], ci->mom_10[i], ci->plus_di[i], ci->plus_dm[i], ci->ppo[i], ci->roc_10[i]);
    fprintf(f, "%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,", ci->stoch.k[i], ci->stoch.d[i], ci->stochf.k[i], ci->stochf.d[i], ci->stochrsi.k[i], ci->stochrsi.d[i], ci->trix[i], ci->ultosc[i], ci->willr_14[i]);
    fprintf(f, "%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,", ci->obv[i], ci->ad[i], ci->adosc[i], ci->bbands.upper[i], ci->bbands.middle[i], ci->bbands.lower[i], ci->atr_14[i], ci->natr_14[i], ci->trange[i]);
    fprintf(f, "%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,", ci->ht_dcperiod[i], ci->ht_dcphase[i], ci->ht_phasor.inphase[i], ci->ht_phasor.quadrature[i], ci->ht_sine.sine[i], ci->ht_sine.leadsine[i], ci->ht_trendmode[i]);
    fprintf(f, "%d,%d,%d,%d,%d,%d,%d\n", ci->cdl_doji[i], ci->cdl_hammer[i], ci->cdl_engulfing[i], ci->cdl_morningstar[i], ci->cdl_eveningstar[i], ci->cdl_3blackcrows[i], ci->cdl_3whitesoldiers[i]);
}

int process_timeframe(const char *symbol, const char *timeframe) {
    char input_directory[1024];
    snprintf(input_directory, sizeof(input_directory), "data/%s/%s", timeframe, symbol);

    char output_directory[1024];
    snprintf(output_directory, sizeof(output_directory), "out/%s", symbol);

    char output_filename[1024];
    snprintf(output_filename, sizeof(output_filename), "%s/%s-%s-out.csv", output_directory, symbol, timeframe);

    ensure_directory_exists("out");
    ensure_directory_exists(output_directory);

    size_t count = 0;
    Candle *candles = parse_csv_directory(input_directory, &count);
    if (!candles) {
        fprintf(stderr, "Failed to load candles for %s %s\n", symbol, timeframe);
        return 1;
    }
    printf("Loaded %zu candles from directory %s\n", count, input_directory);

    FILE *out = fopen(output_filename, "w");
    if (!out) {
        perror("Error opening output file");
        free(candles);
        return 1;
    }

    // Set 10MB buffer for output file
    char *io_buffer = malloc(10 * 1024 * 1024);
    if (io_buffer) {
        if (setvbuf(out, io_buffer, _IOFBF, 10 * 1024 * 1024) != 0) {
            perror("Failed to set buffer");
            // Continue anyway, just unbuffered/default buffered
        } else {
            printf("Output buffering enabled (10MB)\n");
        }
    }

    write_csv_header(out);

    printf("Calculating indicators for %s %s...\n", symbol, timeframe);
    ComputedIndicators *ci = calculate_all_indicators(candles, count);
    if (!ci) {
        fprintf(stderr, "Failed to calculate indicators\n");
        fclose(out);
        free(candles);
        if(io_buffer) free(io_buffer);
        return 1;
    }
    printf("Calculation complete. Writing to file...\n");
    
    for (size_t i = 0; i < count; i++) {
        write_csv_row(out, &candles[i], ci, i);
        
        if (i % 10000 == 0) printf("Processed %zu/%zu\r", i, count);
    }
    printf("Processed %zu/%zu\n", count, count);

    fclose(out);
    free_computed_indicators(ci);
    free(candles);
    if(io_buffer) free(io_buffer);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <symbol> <timeframes>\n", argv[0]);
        printf("Example: %s BTCUSDT 1m,5m,15m\n", argv[0]);
        return 1;
    }

    const char *symbol = argv[1];
    char *timeframes = strdup(argv[2]); // Duplicate because strtok modifies the string
    if (!timeframes) {
        perror("strdup failed");
        return 1;
    }

    char *token = strtok(timeframes, ",");
    while (token != NULL) {
        printf("\nProcessing timeframe: %s\n", token);
        process_timeframe(symbol, token);
        token = strtok(NULL, ",");
    }

    free(timeframes);
    return 0;
}

