#include "indicators.h"
#include <ta-lib/ta_libc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Helper to extract series from candles
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

// Helper to align TA-Lib output to the original timeline
static double* align_double(size_t count, int outBeg, int outNbElement, double *ta_out) {
    double *aligned = calloc(count, sizeof(double));
    if (outNbElement > 0 && outBeg >= 0 && (size_t)outBeg < count) {
        memcpy(aligned + outBeg, ta_out, outNbElement * sizeof(double));
    }
    return aligned;
}

static int* align_int(size_t count, int outBeg, int outNbElement, int *ta_out) {
    int *aligned = calloc(count, sizeof(int));
    if (outNbElement > 0 && outBeg >= 0 && (size_t)outBeg < count) {
        memcpy(aligned + outBeg, ta_out, outNbElement * sizeof(int));
    }
    return aligned;
}

void free_computed_indicators(ComputedIndicators *ci) {
    if (!ci) return;
    if (ci->ema_14) free(ci->ema_14);
    if (ci->sma_14) free(ci->sma_14);
    if (ci->wma_14) free(ci->wma_14);
    if (ci->kama_14) free(ci->kama_14);
    if (ci->tema_14) free(ci->tema_14);
    if (ci->trima_14) free(ci->trima_14);
    if (ci->sar) free(ci->sar);
    if (ci->rsi_14) free(ci->rsi_14);
    if (ci->macd.macd) free(ci->macd.macd);
    if (ci->macd.signal) free(ci->macd.signal);
    if (ci->macd.hist) free(ci->macd.hist);
    if (ci->adx_14) free(ci->adx_14);
    if (ci->adxr_14) free(ci->adxr_14);
    if (ci->apo) free(ci->apo);
    if (ci->aroon.aroon_down) free(ci->aroon.aroon_down);
    if (ci->aroon.aroon_up) free(ci->aroon.aroon_up);
    if (ci->aroonosc) free(ci->aroonosc);
    if (ci->bop) free(ci->bop);
    if (ci->cci_14) free(ci->cci_14);
    if (ci->cmo_14) free(ci->cmo_14);
    if (ci->dx_14) free(ci->dx_14);
    if (ci->mfi_14) free(ci->mfi_14);
    if (ci->minus_di) free(ci->minus_di);
    if (ci->minus_dm) free(ci->minus_dm);
    if (ci->mom_10) free(ci->mom_10);
    if (ci->plus_di) free(ci->plus_di);
    if (ci->plus_dm) free(ci->plus_dm);
    if (ci->ppo) free(ci->ppo);
    if (ci->roc_10) free(ci->roc_10);
    if (ci->stoch.k) free(ci->stoch.k);
    if (ci->stoch.d) free(ci->stoch.d);
    if (ci->stochf.k) free(ci->stochf.k);
    if (ci->stochf.d) free(ci->stochf.d);
    if (ci->stochrsi.k) free(ci->stochrsi.k);
    if (ci->stochrsi.d) free(ci->stochrsi.d);
    if (ci->trix) free(ci->trix);
    if (ci->ultosc) free(ci->ultosc);
    if (ci->willr_14) free(ci->willr_14);
    if (ci->obv) free(ci->obv);
    if (ci->ad) free(ci->ad);
    if (ci->adosc) free(ci->adosc);
    if (ci->bbands.upper) free(ci->bbands.upper);
    if (ci->bbands.middle) free(ci->bbands.middle);
    if (ci->bbands.lower) free(ci->bbands.lower);
    if (ci->atr_14) free(ci->atr_14);
    if (ci->natr_14) free(ci->natr_14);
    if (ci->trange) free(ci->trange);
    if (ci->ht_dcperiod) free(ci->ht_dcperiod);
    if (ci->ht_dcphase) free(ci->ht_dcphase);
    if (ci->ht_phasor.inphase) free(ci->ht_phasor.inphase);
    if (ci->ht_phasor.quadrature) free(ci->ht_phasor.quadrature);
    if (ci->ht_sine.sine) free(ci->ht_sine.sine);
    if (ci->ht_sine.leadsine) free(ci->ht_sine.leadsine);
    if (ci->ht_trendmode) free(ci->ht_trendmode);
    if (ci->cdl_doji) free(ci->cdl_doji);
    if (ci->cdl_hammer) free(ci->cdl_hammer);
    if (ci->cdl_engulfing) free(ci->cdl_engulfing);
    if (ci->cdl_morningstar) free(ci->cdl_morningstar);
    if (ci->cdl_eveningstar) free(ci->cdl_eveningstar);
    if (ci->cdl_3blackcrows) free(ci->cdl_3blackcrows);
    if (ci->cdl_3whitesoldiers) free(ci->cdl_3whitesoldiers);
    free(ci);
}

ComputedIndicators* calculate_all_indicators(Candle *candles, size_t count) {
    if (!candles || count == 0) return NULL;

    ComputedIndicators *ci = calloc(1, sizeof(ComputedIndicators));
    
    double *open = extract_series(candles, count, 'o');
    double *high = extract_series(candles, count, 'h');
    double *low = extract_series(candles, count, 'l');
    double *close = extract_series(candles, count, 'c');
    double *volume = extract_series(candles, count, 'v');

    if (!open || !high || !low || !close || !volume) {
        // Cleanup and return
        if(open) free(open); 
        if(high) free(high); 
        if(low) free(low); 
        if(close) free(close); 
        if(volume) free(volume);
        free(ci);
        return NULL;
    }

    // Temp buffers
    double *tmp1 = malloc(sizeof(double) * count);
    double *tmp2 = malloc(sizeof(double) * count);
    double *tmp3 = malloc(sizeof(double) * count);
    int *tmpInt = malloc(sizeof(int) * count);

    int outBeg, outNbElement;
    TA_RetCode ret;
    (void)ret; // Silence unused variable warning

    // EMA 14
    ret = TA_EMA(0, count - 1, close, 14, &outBeg, &outNbElement, tmp1);
    ci->ema_14 = align_double(count, outBeg, outNbElement, tmp1);

    // SMA 14
    ret = TA_SMA(0, count - 1, close, 14, &outBeg, &outNbElement, tmp1);
    ci->sma_14 = align_double(count, outBeg, outNbElement, tmp1);

    // WMA 14
    ret = TA_WMA(0, count - 1, close, 14, &outBeg, &outNbElement, tmp1);
    ci->wma_14 = align_double(count, outBeg, outNbElement, tmp1);

    // KAMA 14
    ret = TA_KAMA(0, count - 1, close, 14, &outBeg, &outNbElement, tmp1);
    ci->kama_14 = align_double(count, outBeg, outNbElement, tmp1);

    // TEMA 14
    ret = TA_TEMA(0, count - 1, close, 14, &outBeg, &outNbElement, tmp1);
    ci->tema_14 = align_double(count, outBeg, outNbElement, tmp1);

    // TRIMA 14
    ret = TA_TRIMA(0, count - 1, close, 14, &outBeg, &outNbElement, tmp1);
    ci->trima_14 = align_double(count, outBeg, outNbElement, tmp1);

    // SAR
    ret = TA_SAR(0, count - 1, high, low, 0.02, 0.2, &outBeg, &outNbElement, tmp1);
    ci->sar = align_double(count, outBeg, outNbElement, tmp1);

    // RSI 14
    ret = TA_RSI(0, count - 1, close, 14, &outBeg, &outNbElement, tmp1);
    ci->rsi_14 = align_double(count, outBeg, outNbElement, tmp1);

    // MACD
    ret = TA_MACD(0, count - 1, close, 12, 26, 9, &outBeg, &outNbElement, tmp1, tmp2, tmp3);
    ci->macd.macd = align_double(count, outBeg, outNbElement, tmp1);
    ci->macd.signal = align_double(count, outBeg, outNbElement, tmp2);
    ci->macd.hist = align_double(count, outBeg, outNbElement, tmp3);

    // ADX 14
    ret = TA_ADX(0, count - 1, high, low, close, 14, &outBeg, &outNbElement, tmp1);
    ci->adx_14 = align_double(count, outBeg, outNbElement, tmp1);

    // ADXR 14
    ret = TA_ADXR(0, count - 1, high, low, close, 14, &outBeg, &outNbElement, tmp1);
    ci->adxr_14 = align_double(count, outBeg, outNbElement, tmp1);

    // APO
    ret = TA_APO(0, count - 1, close, 12, 26, TA_MAType_SMA, &outBeg, &outNbElement, tmp1);
    ci->apo = align_double(count, outBeg, outNbElement, tmp1);

    // AROON
    ret = TA_AROON(0, count - 1, high, low, 14, &outBeg, &outNbElement, tmp1, tmp2);
    ci->aroon.aroon_down = align_double(count, outBeg, outNbElement, tmp1);
    ci->aroon.aroon_up = align_double(count, outBeg, outNbElement, tmp2);

    // AROONOSC
    ret = TA_AROONOSC(0, count - 1, high, low, 14, &outBeg, &outNbElement, tmp1);
    ci->aroonosc = align_double(count, outBeg, outNbElement, tmp1);

    // BOP
    ret = TA_BOP(0, count - 1, open, high, low, close, &outBeg, &outNbElement, tmp1);
    ci->bop = align_double(count, outBeg, outNbElement, tmp1);

    // CCI 14
    ret = TA_CCI(0, count - 1, high, low, close, 14, &outBeg, &outNbElement, tmp1);
    ci->cci_14 = align_double(count, outBeg, outNbElement, tmp1);

    // CMO 14
    ret = TA_CMO(0, count - 1, close, 14, &outBeg, &outNbElement, tmp1);
    ci->cmo_14 = align_double(count, outBeg, outNbElement, tmp1);

    // DX 14
    ret = TA_DX(0, count - 1, high, low, close, 14, &outBeg, &outNbElement, tmp1);
    ci->dx_14 = align_double(count, outBeg, outNbElement, tmp1);

    // MFI 14
    ret = TA_MFI(0, count - 1, high, low, close, volume, 14, &outBeg, &outNbElement, tmp1);
    ci->mfi_14 = align_double(count, outBeg, outNbElement, tmp1);

    // MINUS_DI
    ret = TA_MINUS_DI(0, count - 1, high, low, close, 14, &outBeg, &outNbElement, tmp1);
    ci->minus_di = align_double(count, outBeg, outNbElement, tmp1);

    // MINUS_DM
    ret = TA_MINUS_DM(0, count - 1, high, low, 14, &outBeg, &outNbElement, tmp1);
    ci->minus_dm = align_double(count, outBeg, outNbElement, tmp1);

    // MOM 10
    ret = TA_MOM(0, count - 1, close, 10, &outBeg, &outNbElement, tmp1);
    ci->mom_10 = align_double(count, outBeg, outNbElement, tmp1);

    // PLUS_DI
    ret = TA_PLUS_DI(0, count - 1, high, low, close, 14, &outBeg, &outNbElement, tmp1);
    ci->plus_di = align_double(count, outBeg, outNbElement, tmp1);

    // PLUS_DM
    ret = TA_PLUS_DM(0, count - 1, high, low, 14, &outBeg, &outNbElement, tmp1);
    ci->plus_dm = align_double(count, outBeg, outNbElement, tmp1);

    // PPO
    ret = TA_PPO(0, count - 1, close, 12, 26, TA_MAType_SMA, &outBeg, &outNbElement, tmp1);
    ci->ppo = align_double(count, outBeg, outNbElement, tmp1);

    // ROC 10
    ret = TA_ROC(0, count - 1, close, 10, &outBeg, &outNbElement, tmp1);
    ci->roc_10 = align_double(count, outBeg, outNbElement, tmp1);

    // STOCH
    ret = TA_STOCH(0, count - 1, high, low, close, 5, 3, TA_MAType_SMA, 3, TA_MAType_SMA, &outBeg, &outNbElement, tmp1, tmp2);
    ci->stoch.k = align_double(count, outBeg, outNbElement, tmp1);
    ci->stoch.d = align_double(count, outBeg, outNbElement, tmp2);

    // STOCHF
    ret = TA_STOCHF(0, count - 1, high, low, close, 5, 3, TA_MAType_SMA, &outBeg, &outNbElement, tmp1, tmp2);
    ci->stochf.k = align_double(count, outBeg, outNbElement, tmp1);
    ci->stochf.d = align_double(count, outBeg, outNbElement, tmp2);

    // STOCHRSI
    ret = TA_STOCHRSI(0, count - 1, close, 14, 5, 3, TA_MAType_SMA, &outBeg, &outNbElement, tmp1, tmp2);
    ci->stochrsi.k = align_double(count, outBeg, outNbElement, tmp1);
    ci->stochrsi.d = align_double(count, outBeg, outNbElement, tmp2);

    // TRIX
    ret = TA_TRIX(0, count - 1, close, 30, &outBeg, &outNbElement, tmp1);
    ci->trix = align_double(count, outBeg, outNbElement, tmp1);

    // ULTOSC
    ret = TA_ULTOSC(0, count - 1, high, low, close, 7, 14, 28, &outBeg, &outNbElement, tmp1);
    ci->ultosc = align_double(count, outBeg, outNbElement, tmp1);

    // WILLR 14
    ret = TA_WILLR(0, count - 1, high, low, close, 14, &outBeg, &outNbElement, tmp1);
    ci->willr_14 = align_double(count, outBeg, outNbElement, tmp1);

    // OBV
    ret = TA_OBV(0, count - 1, close, volume, &outBeg, &outNbElement, tmp1);
    ci->obv = align_double(count, outBeg, outNbElement, tmp1);

    // AD
    ret = TA_AD(0, count - 1, high, low, close, volume, &outBeg, &outNbElement, tmp1);
    ci->ad = align_double(count, outBeg, outNbElement, tmp1);

    // ADOSC
    ret = TA_ADOSC(0, count - 1, high, low, close, volume, 3, 10, &outBeg, &outNbElement, tmp1);
    ci->adosc = align_double(count, outBeg, outNbElement, tmp1);

    // BBANDS
    ret = TA_BBANDS(0, count - 1, close, 20, 2.0, 2.0, TA_MAType_SMA, &outBeg, &outNbElement, tmp1, tmp2, tmp3);
    ci->bbands.upper = align_double(count, outBeg, outNbElement, tmp1);
    ci->bbands.middle = align_double(count, outBeg, outNbElement, tmp2);
    ci->bbands.lower = align_double(count, outBeg, outNbElement, tmp3);

    // ATR 14
    ret = TA_ATR(0, count - 1, high, low, close, 14, &outBeg, &outNbElement, tmp1);
    ci->atr_14 = align_double(count, outBeg, outNbElement, tmp1);

    // NATR 14
    ret = TA_NATR(0, count - 1, high, low, close, 14, &outBeg, &outNbElement, tmp1);
    ci->natr_14 = align_double(count, outBeg, outNbElement, tmp1);

    // TRANGE
    ret = TA_TRANGE(0, count - 1, high, low, close, &outBeg, &outNbElement, tmp1);
    ci->trange = align_double(count, outBeg, outNbElement, tmp1);

    // HT_DCPERIOD
    ret = TA_HT_DCPERIOD(0, count - 1, close, &outBeg, &outNbElement, tmp1);
    ci->ht_dcperiod = align_double(count, outBeg, outNbElement, tmp1);

    // HT_DCPHASE
    ret = TA_HT_DCPHASE(0, count - 1, close, &outBeg, &outNbElement, tmp1);
    ci->ht_dcphase = align_double(count, outBeg, outNbElement, tmp1);

    // HT_PHASOR
    ret = TA_HT_PHASOR(0, count - 1, close, &outBeg, &outNbElement, tmp1, tmp2);
    ci->ht_phasor.inphase = align_double(count, outBeg, outNbElement, tmp1);
    ci->ht_phasor.quadrature = align_double(count, outBeg, outNbElement, tmp2);

    // HT_SINE
    ret = TA_HT_SINE(0, count - 1, close, &outBeg, &outNbElement, tmp1, tmp2);
    ci->ht_sine.sine = align_double(count, outBeg, outNbElement, tmp1);
    ci->ht_sine.leadsine = align_double(count, outBeg, outNbElement, tmp2);

    // HT_TRENDMODE (returns int)
    ret = TA_HT_TRENDMODE(0, count - 1, close, &outBeg, &outNbElement, tmpInt);
    // Convert int array to double array for consistency with struct
    ci->ht_trendmode = calloc(count, sizeof(double));
    if (outNbElement > 0) {
        for(int i=0; i<outNbElement; i++) {
            ci->ht_trendmode[outBeg + i] = (double)tmpInt[i];
        }
    }

    // CDL Patterns (return int)
    ret = TA_CDLDOJI(0, count - 1, open, high, low, close, &outBeg, &outNbElement, tmpInt);
    ci->cdl_doji = align_int(count, outBeg, outNbElement, tmpInt);

    ret = TA_CDLHAMMER(0, count - 1, open, high, low, close, &outBeg, &outNbElement, tmpInt);
    ci->cdl_hammer = align_int(count, outBeg, outNbElement, tmpInt);

    ret = TA_CDLENGULFING(0, count - 1, open, high, low, close, &outBeg, &outNbElement, tmpInt);
    ci->cdl_engulfing = align_int(count, outBeg, outNbElement, tmpInt);

    ret = TA_CDLMORNINGSTAR(0, count - 1, open, high, low, close, 0.3, &outBeg, &outNbElement, tmpInt);
    ci->cdl_morningstar = align_int(count, outBeg, outNbElement, tmpInt);

    ret = TA_CDLEVENINGSTAR(0, count - 1, open, high, low, close, 0.3, &outBeg, &outNbElement, tmpInt);
    ci->cdl_eveningstar = align_int(count, outBeg, outNbElement, tmpInt);

    ret = TA_CDL3BLACKCROWS(0, count - 1, open, high, low, close, &outBeg, &outNbElement, tmpInt);
    ci->cdl_3blackcrows = align_int(count, outBeg, outNbElement, tmpInt);

    ret = TA_CDL3WHITESOLDIERS(0, count - 1, open, high, low, close, &outBeg, &outNbElement, tmpInt);
    ci->cdl_3whitesoldiers = align_int(count, outBeg, outNbElement, tmpInt);

    // Cleanup
    free(tmp1); free(tmp2); free(tmp3); free(tmpInt);
    free(open); free(high); free(low); free(close); free(volume);

    return ci;
}
