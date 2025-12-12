#include "indicators.h"
#include "../config/config.h"
#include <ta-lib/ta_libc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static double* extract_series(Candle *candles, size_t count, char field) {
    double *series = malloc(sizeof(double) * count);
    if (!series) return NULL;
    
    for (size_t i = 0; i < count; i++) {
        switch (field) {
            case 'o': series[i] = candles[i].o; break;
            case 'h': series[i] = candles[i].h; break;
            case 'l': series[i] = candles[i].l; break;
            case 'c': series[i] = candles[i].c; break;
            case 'v': series[i] = candles[i].v; break;
        }
    }
    return series;
}

static int get_int_param(cJSON *item, const char *key, int default_val) {
    if (cJSON_IsNumber(item)) return item->valueint;
    if (cJSON_IsObject(item)) {
        cJSON *val = cJSON_GetObjectItem(item, key);
        if (val && cJSON_IsNumber(val)) return val->valueint;
    }
    return default_val;
}

static double get_double_param(cJSON *item, const char *key, double default_val) {
    if (cJSON_IsNumber(item)) return item->valuedouble;
    if (cJSON_IsObject(item)) {
        cJSON *val = cJSON_GetObjectItem(item, key);
        if (val && cJSON_IsNumber(val)) return val->valuedouble;
    }
    return default_val;
}

static char* get_string_param(cJSON *item, const char *key, char *default_val) {
    if (cJSON_IsObject(item)) {
        cJSON *val = cJSON_GetObjectItem(item, key);
        if (val && cJSON_IsString(val)) return val->valuestring;
    }
    return default_val;
}

static double* get_series_by_name(char *type, double *open, double *high, double *low, double *close, double *volume) {
    if (strcmp(type, "open") == 0) return open;
    if (strcmp(type, "high") == 0) return high;
    if (strcmp(type, "low") == 0) return low;
    if (strcmp(type, "volume") == 0) return volume;
    return close;
}

cJSON* calculate_indicators(Candle *candles, size_t count) {
    if (!candles || count < 30) return NULL;
    if (!app_config.json) return NULL;

    cJSON *indicators_config = cJSON_GetObjectItem(app_config.json, "indicators");
    if (!indicators_config) return NULL;

    double *open = extract_series(candles, count, 'o');
    double *high = extract_series(candles, count, 'h');
    double *low = extract_series(candles, count, 'l');
    double *close = extract_series(candles, count, 'c');
    double *volume = extract_series(candles, count, 'v');

    if (!open || !high || !low || !close || !volume) {
        if(open) free(open);
        if(high) free(high);
        if(low) free(low);
        if(close) free(close);
        if(volume) free(volume);
        return NULL;
    }

    cJSON *result = cJSON_CreateObject();
    cJSON *indicator_type = NULL;

    cJSON_ArrayForEach(indicator_type, indicators_config) {
        char *type = indicator_type->string;
        cJSON *outputs = indicator_type;
        
        cJSON *output = NULL;
        cJSON_ArrayForEach(output, outputs) {
            char *output_name = output->string;
            int outBeg, outNbElement;
            TA_RetCode ret = TA_SUCCESS;
            double *outReal = malloc(sizeof(double) * count);
            
            if (strcmp(type, "EMA") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_EMA(0, count - 1, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "SMA") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_SMA(0, count - 1, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "WMA") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_WMA(0, count - 1, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "KAMA") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_KAMA(0, count - 1, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "TEMA") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_TEMA(0, count - 1, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "TRIMA") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_TRIMA(0, count - 1, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "SAR") == 0) {
                double accel = get_double_param(output, "acceleration", 0.02);
                double max = get_double_param(output, "maximum", 0.2);
                ret = TA_SAR(0, count - 1, high, low, accel, max, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "RSI") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_RSI(0, count - 1, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "ADX") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_ADX(0, count - 1, high, low, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "ADXR") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_ADXR(0, count - 1, high, low, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "APO") == 0) {
                int fast = get_int_param(output, "fast", 12);
                int slow = get_int_param(output, "slow", 26);
                ret = TA_APO(0, count - 1, close, fast, slow, TA_MAType_SMA, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "AROON") == 0) {
                int period = get_int_param(output, "period", 14);
                double *outAroonDown = malloc(sizeof(double) * count);
                double *outAroonUp = malloc(sizeof(double) * count);
                ret = TA_AROON(0, count - 1, high, low, period, &outBeg, &outNbElement, outAroonDown, outAroonUp);
                if (ret == TA_SUCCESS && outNbElement > 0) {
                    cJSON *aroon = cJSON_CreateObject();
                    cJSON_AddNumberToObject(aroon, "down", outAroonDown[outNbElement - 1]);
                    cJSON_AddNumberToObject(aroon, "up", outAroonUp[outNbElement - 1]);
                    cJSON_AddItemToObject(result, output_name, aroon);
                }
                free(outAroonDown);
                free(outAroonUp);
                ret = TA_INTERNAL_ERROR;
            }
            else if (strcmp(type, "AROONOSC") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_AROONOSC(0, count - 1, high, low, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "BOP") == 0) {
                ret = TA_BOP(0, count - 1, open, high, low, close, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "CCI") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_CCI(0, count - 1, high, low, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "CMO") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_CMO(0, count - 1, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "DX") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_DX(0, count - 1, high, low, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "MFI") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_MFI(0, count - 1, high, low, close, volume, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "MINUS_DI") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_MINUS_DI(0, count - 1, high, low, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "MINUS_DM") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_MINUS_DM(0, count - 1, high, low, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "MOM") == 0) {
                int period = get_int_param(output, "period", 10);
                ret = TA_MOM(0, count - 1, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "PLUS_DI") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_PLUS_DI(0, count - 1, high, low, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "PLUS_DM") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_PLUS_DM(0, count - 1, high, low, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "PPO") == 0) {
                int fast = get_int_param(output, "fast", 12);
                int slow = get_int_param(output, "slow", 26);
                ret = TA_PPO(0, count - 1, close, fast, slow, TA_MAType_SMA, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "ROC") == 0) {
                int period = get_int_param(output, "period", 10);
                ret = TA_ROC(0, count - 1, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "TRIX") == 0) {
                int period = get_int_param(output, "period", 30);
                ret = TA_TRIX(0, count - 1, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "ULTOSC") == 0) {
                int t1 = get_int_param(output, "timeperiod1", 7);
                int t2 = get_int_param(output, "timeperiod2", 14);
                int t3 = get_int_param(output, "timeperiod3", 28);
                ret = TA_ULTOSC(0, count - 1, high, low, close, t1, t2, t3, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "WILLR") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_WILLR(0, count - 1, high, low, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "MACD") == 0) {
                int fast = get_int_param(output, "fast", 12);
                int slow = get_int_param(output, "slow", 26);
                int signal = get_int_param(output, "signal", 9);
                double *outMACD = malloc(sizeof(double) * count);
                double *outMACDSignal = malloc(sizeof(double) * count);
                double *outMACDHist = malloc(sizeof(double) * count);
                ret = TA_MACD(0, count - 1, close, fast, slow, signal, &outBeg, &outNbElement, outMACD, outMACDSignal, outMACDHist);
                if (ret == TA_SUCCESS && outNbElement > 0) {
                    cJSON *macd = cJSON_CreateObject();
                    cJSON_AddNumberToObject(macd, "macd", outMACD[outNbElement - 1]);
                    cJSON_AddNumberToObject(macd, "signal", outMACDSignal[outNbElement - 1]);
                    cJSON_AddNumberToObject(macd, "hist", outMACDHist[outNbElement - 1]);
                    cJSON_AddItemToObject(result, output_name, macd);
                }
                free(outMACD); free(outMACDSignal); free(outMACDHist);
                ret = TA_INTERNAL_ERROR;
            }
            else if (strcmp(type, "STOCH") == 0) {
                int fastk = get_int_param(output, "fastk_period", 5);
                int slowk = get_int_param(output, "slowk_period", 3);
                int slowd = get_int_param(output, "slowd_period", 3);
                double *outSlowK = malloc(sizeof(double) * count);
                double *outSlowD = malloc(sizeof(double) * count);
                ret = TA_STOCH(0, count - 1, high, low, close, fastk, slowk, TA_MAType_SMA, slowd, TA_MAType_SMA, &outBeg, &outNbElement, outSlowK, outSlowD);
                if (ret == TA_SUCCESS && outNbElement > 0) {
                    cJSON *stoch = cJSON_CreateObject();
                    cJSON_AddNumberToObject(stoch, "k", outSlowK[outNbElement - 1]);
                    cJSON_AddNumberToObject(stoch, "d", outSlowD[outNbElement - 1]);
                    cJSON_AddItemToObject(result, output_name, stoch);
                }
                free(outSlowK); free(outSlowD);
                ret = TA_INTERNAL_ERROR;
            }
            else if (strcmp(type, "STOCHF") == 0) {
                int fastk = get_int_param(output, "fastk_period", 5);
                int fastd = get_int_param(output, "fastd_period", 3);
                double *outFastK = malloc(sizeof(double) * count);
                double *outFastD = malloc(sizeof(double) * count);
                ret = TA_STOCHF(0, count - 1, high, low, close, fastk, fastd, TA_MAType_SMA, &outBeg, &outNbElement, outFastK, outFastD);
                if (ret == TA_SUCCESS && outNbElement > 0) {
                    cJSON *stochf = cJSON_CreateObject();
                    cJSON_AddNumberToObject(stochf, "k", outFastK[outNbElement - 1]);
                    cJSON_AddNumberToObject(stochf, "d", outFastD[outNbElement - 1]);
                    cJSON_AddItemToObject(result, output_name, stochf);
                }
                free(outFastK); free(outFastD);
                ret = TA_INTERNAL_ERROR;
            }
            else if (strcmp(type, "STOCHRSI") == 0) {
                int period = get_int_param(output, "period", 14);
                int fastk = get_int_param(output, "fastk_period", 5);
                int fastd = get_int_param(output, "fastd_period", 3);
                double *outSlowK = malloc(sizeof(double) * count);
                double *outSlowD = malloc(sizeof(double) * count);
                ret = TA_STOCHRSI(0, count - 1, close, period, fastk, fastd, TA_MAType_SMA, &outBeg, &outNbElement, outSlowK, outSlowD);
                if (ret == TA_SUCCESS && outNbElement > 0) {
                    cJSON *stochrsi = cJSON_CreateObject();
                    cJSON_AddNumberToObject(stochrsi, "k", outSlowK[outNbElement - 1]);
                    cJSON_AddNumberToObject(stochrsi, "d", outSlowD[outNbElement - 1]);
                    cJSON_AddItemToObject(result, output_name, stochrsi);
                }
                free(outSlowK); free(outSlowD);
                ret = TA_INTERNAL_ERROR;
            }
            else if (strcmp(type, "OBV") == 0) {
                ret = TA_OBV(0, count - 1, close, volume, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "AD") == 0) {
                ret = TA_AD(0, count - 1, high, low, close, volume, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "ADOSC") == 0) {
                int fast = get_int_param(output, "fast", 3);
                int slow = get_int_param(output, "slow", 10);
                ret = TA_ADOSC(0, count - 1, high, low, close, volume, fast, slow, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "BBANDS") == 0) {
                int period = get_int_param(output, "period", 20);
                double devup = get_double_param(output, "devup", 2.0);
                double devdn = get_double_param(output, "devdn", 2.0);
                double *outUpper = malloc(sizeof(double) * count);
                double *outMiddle = malloc(sizeof(double) * count);
                double *outLower = malloc(sizeof(double) * count);
                ret = TA_BBANDS(0, count - 1, close, period, devup, devdn, TA_MAType_SMA, &outBeg, &outNbElement, outUpper, outMiddle, outLower);
                if (ret == TA_SUCCESS && outNbElement > 0) {
                    cJSON *bbands = cJSON_CreateObject();
                    double u = outUpper[outNbElement - 1];
                    double m = outMiddle[outNbElement - 1];
                    double l = outLower[outNbElement - 1];
                    double c = close[count - 1];
                    
                    cJSON_AddNumberToObject(bbands, "upper", u);
                    cJSON_AddNumberToObject(bbands, "middle", m);
                    cJSON_AddNumberToObject(bbands, "lower", l);
                    
                    // Derived values
                    cJSON_AddNumberToObject(bbands, "bandwidth", (m != 0) ? ((u - l) / m * 100.0) : 0);
                    cJSON_AddNumberToObject(bbands, "percent_b", (u != l) ? ((c - l) / (u - l)) : 0);
                    cJSON_AddNumberToObject(bbands, "delta", fabs(m - l));
                    cJSON_AddNumberToObject(bbands, "tail", fabs(c - l));

                    cJSON_AddItemToObject(result, output_name, bbands);
                }
                free(outUpper); free(outMiddle); free(outLower);
                ret = TA_INTERNAL_ERROR; // Skip default handling
            }
            else if (strcmp(type, "ATR") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_ATR(0, count - 1, high, low, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "NATR") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_NATR(0, count - 1, high, low, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "TRANGE") == 0) {
                ret = TA_TRANGE(0, count - 1, high, low, close, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "HT_DCPERIOD") == 0) {
                ret = TA_HT_DCPERIOD(0, count - 1, close, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "HT_DCPHASE") == 0) {
                ret = TA_HT_DCPHASE(0, count - 1, close, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "HT_TRENDMODE") == 0) {
                int *outInteger = malloc(sizeof(int) * count);
                ret = TA_HT_TRENDMODE(0, count - 1, close, &outBeg, &outNbElement, outInteger);
                if (ret == TA_SUCCESS && outNbElement > 0) cJSON_AddNumberToObject(result, output_name, outInteger[outNbElement - 1]);
                free(outInteger);
                ret = TA_INTERNAL_ERROR;
            }
            else if (strcmp(type, "HT_PHASOR") == 0) {
                double *outInPhase = malloc(sizeof(double) * count);
                double *outQuadrature = malloc(sizeof(double) * count);
                ret = TA_HT_PHASOR(0, count - 1, close, &outBeg, &outNbElement, outInPhase, outQuadrature);
                if (ret == TA_SUCCESS && outNbElement > 0) {
                    cJSON *phasor = cJSON_CreateObject();
                    cJSON_AddNumberToObject(phasor, "inphase", outInPhase[outNbElement - 1]);
                    cJSON_AddNumberToObject(phasor, "quadrature", outQuadrature[outNbElement - 1]);
                    cJSON_AddItemToObject(result, output_name, phasor);
                }
                free(outInPhase); free(outQuadrature);
                ret = TA_INTERNAL_ERROR;
            }
            else if (strcmp(type, "HT_SINE") == 0) {
                double *outSine = malloc(sizeof(double) * count);
                double *outLeadSine = malloc(sizeof(double) * count);
                ret = TA_HT_SINE(0, count - 1, close, &outBeg, &outNbElement, outSine, outLeadSine);
                if (ret == TA_SUCCESS && outNbElement > 0) {
                    cJSON *sine = cJSON_CreateObject();
                    cJSON_AddNumberToObject(sine, "sine", outSine[outNbElement - 1]);
                    cJSON_AddNumberToObject(sine, "leadsine", outLeadSine[outNbElement - 1]);
                    cJSON_AddItemToObject(result, output_name, sine);
                }
                free(outSine); free(outLeadSine);
                ret = TA_INTERNAL_ERROR;
            }
            else if (strncmp(type, "CDL", 3) == 0) {
                int *outInt = malloc(sizeof(int) * count);
                if (strcmp(type, "CDLDOJI") == 0) ret = TA_CDLDOJI(0, count - 1, open, high, low, close, &outBeg, &outNbElement, outInt);
                else if (strcmp(type, "CDLHAMMER") == 0) ret = TA_CDLHAMMER(0, count - 1, open, high, low, close, &outBeg, &outNbElement, outInt);
                else if (strcmp(type, "CDLENGULFING") == 0) ret = TA_CDLENGULFING(0, count - 1, open, high, low, close, &outBeg, &outNbElement, outInt);
                else if (strcmp(type, "CDLMORNINGSTAR") == 0) ret = TA_CDLMORNINGSTAR(0, count - 1, open, high, low, close, 0.3, &outBeg, &outNbElement, outInt);
                else if (strcmp(type, "CDLEVENINGSTAR") == 0) ret = TA_CDLEVENINGSTAR(0, count - 1, open, high, low, close, 0.3, &outBeg, &outNbElement, outInt);
                else if (strcmp(type, "CDL3BLACKCROWS") == 0) ret = TA_CDL3BLACKCROWS(0, count - 1, open, high, low, close, &outBeg, &outNbElement, outInt);
                else if (strcmp(type, "CDL3WHITESOLDIERS") == 0) ret = TA_CDL3WHITESOLDIERS(0, count - 1, open, high, low, close, &outBeg, &outNbElement, outInt);
                
                if (ret == TA_SUCCESS && outNbElement > 0) cJSON_AddNumberToObject(result, output_name, outInt[outNbElement - 1]);
                free(outInt);
                ret = TA_INTERNAL_ERROR;
            }
            else if (strcmp(type, "AROON") == 0) {
                int period = get_int_param(output, "period", 14);
                double *outAroonDown = malloc(sizeof(double) * count);
                double *outAroonUp = malloc(sizeof(double) * count);
                ret = TA_AROON(0, count - 1, high, low, period, &outBeg, &outNbElement, outAroonDown, outAroonUp);
                if (ret == TA_SUCCESS && outNbElement > 0) {
                    cJSON *aroon = cJSON_CreateObject();
                    cJSON_AddNumberToObject(aroon, "down", outAroonDown[outNbElement - 1]);
                    cJSON_AddNumberToObject(aroon, "up", outAroonUp[outNbElement - 1]);
                    cJSON_AddItemToObject(result, output_name, aroon);
                }
                free(outAroonDown); free(outAroonUp);
                ret = TA_INTERNAL_ERROR;
            }
            else if (strcmp(type, "AROONOSC") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_AROONOSC(0, count - 1, high, low, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "BOP") == 0) {
                ret = TA_BOP(0, count - 1, open, high, low, close, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "CCI") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_CCI(0, count - 1, high, low, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "CMO") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_CMO(0, count - 1, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "DX") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_DX(0, count - 1, high, low, close, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "MFI") == 0) {
                int period = get_int_param(output, "period", 14);
                ret = TA_MFI(0, count - 1, high, low, close, volume, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "CMF") == 0) {
                int period = get_int_param(output, "period", 20);
                double cmf_sum = 0.0;
                double vol_sum = 0.0;
                size_t start_idx = (count > (size_t)period) ? count - period : 0;
                for (size_t i = start_idx; i < count; i++) {
                    double hlc3 = ((high[i] - low[i]) != 0) ? ((close[i] - low[i]) - (high[i] - close[i])) / (high[i] - low[i]) : 0;
                    cmf_sum += hlc3 * volume[i];
                    vol_sum += volume[i];
                }
                double cmf = (vol_sum != 0) ? (cmf_sum / vol_sum) : 0;
                cJSON_AddNumberToObject(result, output_name, cmf);
                ret = TA_INTERNAL_ERROR;
            }
            else if (strcmp(type, "MAX") == 0) {
                int period = get_int_param(output, "period", 14);
                char *source = get_string_param(output, "source", "close");
                double *src = get_series_by_name(source, open, high, low, close, volume);
                ret = TA_MAX(0, count - 1, src, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "MIN") == 0) {
                int period = get_int_param(output, "period", 14);
                char *source = get_string_param(output, "source", "close");
                double *src = get_series_by_name(source, open, high, low, close, volume);
                ret = TA_MIN(0, count - 1, src, period, &outBeg, &outNbElement, outReal);
            }
            else if (strcmp(type, "NUM_EMPTY") == 0) {
                int period = get_int_param(output, "period", 288);
                int empty_count = 0;
                size_t start_idx = (count > (size_t)period) ? count - period : 0;
                for (size_t i = start_idx; i < count; i++) {
                    if (volume[i] <= 0) empty_count++;
                }
                cJSON_AddNumberToObject(result, output_name, empty_count);
                ret = TA_INTERNAL_ERROR;
            }
            else if (strcmp(type, "CLOSE_DELTA") == 0) {
                double delta = (count >= 2) ? fabs(close[count - 1] - close[count - 2]) : 0;
                cJSON_AddNumberToObject(result, output_name, delta);
                ret = TA_INTERNAL_ERROR;
            }
            else if (strcmp(type, "CANDLE_CHANGE") == 0) {
                double change = (open[count - 1] != 0) ? ((close[count - 1] - open[count - 1]) / open[count - 1] * 100.0) : 0;
                cJSON_AddNumberToObject(result, output_name, change);
                ret = TA_INTERNAL_ERROR;
            }
            else if (strcmp(type, "KST") == 0) {
                int r1 = get_int_param(output, "roc1", 10);
                int r2 = get_int_param(output, "roc2", 15);
                int r3 = get_int_param(output, "roc3", 20);
                int r4 = get_int_param(output, "roc4", 30);
                
                int s1 = get_int_param(output, "sma1", 10);
                int s2 = get_int_param(output, "sma2", 10);
                int s3 = get_int_param(output, "sma3", 10);
                int s4 = get_int_param(output, "sma4", 15);
                
                int sig_p = get_int_param(output, "signal", 9);

                double *roc1 = malloc(sizeof(double) * count);
                double *roc2 = malloc(sizeof(double) * count);
                double *roc3 = malloc(sizeof(double) * count);
                double *roc4 = malloc(sizeof(double) * count);
                
                double *rc1 = malloc(sizeof(double) * count);
                double *rc2 = malloc(sizeof(double) * count);
                double *rc3 = malloc(sizeof(double) * count);
                double *rc4 = malloc(sizeof(double) * count);

                int b1, n1, b2, n2, b3, n3, b4, n4;
                int sb1, sn1, sb2, sn2, sb3, sn3, sb4, sn4;

                // Calculate ROCs
                TA_ROC(0, count-1, close, r1, &b1, &n1, roc1);
                TA_ROC(0, count-1, close, r2, &b2, &n2, roc2);
                TA_ROC(0, count-1, close, r3, &b3, &n3, roc3);
                TA_ROC(0, count-1, close, r4, &b4, &n4, roc4);

                // Calculate SMAs of ROCs
                TA_SMA(0, n1-1, roc1, s1, &sb1, &sn1, rc1);
                TA_SMA(0, n2-1, roc2, s2, &sb2, &sn2, rc2);
                TA_SMA(0, n3-1, roc3, s3, &sb3, &sn3, rc3);
                TA_SMA(0, n4-1, roc4, s4, &sb4, &sn4, rc4);

                // Find common length
                int common_len = sn1;
                if (sn2 < common_len) common_len = sn2;
                if (sn3 < common_len) common_len = sn3;
                if (sn4 < common_len) common_len = sn4;

                if (common_len > 0) {
                    double *kst_series = malloc(sizeof(double) * common_len);
                    for (int i = 0; i < common_len; i++) {
                        // Align to end
                        double v1 = rc1[sn1 - common_len + i];
                        double v2 = rc2[sn2 - common_len + i];
                        double v3 = rc3[sn3 - common_len + i];
                        double v4 = rc4[sn4 - common_len + i];
                        kst_series[i] = (v1 * 1.0) + (v2 * 2.0) + (v3 * 3.0) + (v4 * 4.0);
                    }

                    // Calculate Signal
                    double *signal_out = malloc(sizeof(double) * common_len);
                    int sig_b, sig_n;
                    TA_SMA(0, common_len-1, kst_series, sig_p, &sig_b, &sig_n, signal_out);

                    if (sig_n > 0) {
                        cJSON *kst_obj = cJSON_CreateObject();
                        cJSON_AddNumberToObject(kst_obj, "kst", kst_series[common_len - 1]);
                        cJSON_AddNumberToObject(kst_obj, "signal", signal_out[sig_n - 1]);
                        cJSON_AddItemToObject(result, output_name, kst_obj);
                    }
                    
                    free(kst_series);
                    free(signal_out);
                }

                free(roc1); free(roc2); free(roc3); free(roc4);
                free(rc1); free(rc2); free(rc3); free(rc4);
                
                ret = TA_INTERNAL_ERROR; // Skip default handling
            }

            // Generic handling for single-value indicators that support change_pct
            if (ret == TA_SUCCESS && outNbElement > 0) {
                double val = outReal[outNbElement - 1];
                char *output_mode = get_string_param(output, "output", "value");
                if (strcmp(output_mode, "change_pct") == 0) {
                    if (outNbElement >= 2) {
                        double prev = outReal[outNbElement - 2];
                        val = (prev != 0) ? ((val - prev) / fabs(prev) * 100.0) : 0;
                    } else {
                        val = 0;
                    }
                }
                cJSON_AddNumberToObject(result, output_name, val);
            }

            free(outReal);
        }
    }

    free(open);
    free(high);
    free(low);
    free(close);
    free(volume);
    
    return result;
}
