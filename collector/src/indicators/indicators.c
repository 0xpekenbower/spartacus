#include "indicators.h"
#include <ta-lib/ta_libc.h>
#include <stdlib.h>
#include <stdio.h>

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

void calculate_indicators(Candle *candles, size_t count, Indicators *out) {
    if (!out) return;
    out->valid = 0;
    if (!candles || count < 30) return; 

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
        return;
    }

    int outBeg, outNbElement;
    TA_RetCode ret;

    double *outReal = malloc(sizeof(double) * count);
    ret = TA_EMA(0, count - 1, close, 14, &outBeg, &outNbElement, outReal);
    out->ema_14 = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_SMA(0, count - 1, close, 14, &outBeg, &outNbElement, outReal);
    out->sma_14 = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_WMA(0, count - 1, close, 14, &outBeg, &outNbElement, outReal);
    out->wma_14 = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_KAMA(0, count - 1, close, 14, &outBeg, &outNbElement, outReal);
    out->kama_14 = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_TEMA(0, count - 1, close, 14, &outBeg, &outNbElement, outReal);
    out->tema_14 = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_TRIMA(0, count - 1, close, 14, &outBeg, &outNbElement, outReal);
    out->trima_14 = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_SAR(0, count - 1, high, low, 0.02, 0.2, &outBeg, &outNbElement, outReal);
    out->sar = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_RSI(0, count - 1, close, 14, &outBeg, &outNbElement, outReal);
    out->rsi_14 = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_ADX(0, count - 1, high, low, close, 14, &outBeg, &outNbElement, outReal);
    out->adx_14 = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_ADXR(0, count - 1, high, low, close, 14, &outBeg, &outNbElement, outReal);
    out->adxr_14 = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_APO(0, count - 1, close, 12, 26, TA_MAType_SMA, &outBeg, &outNbElement, outReal);
    out->apo = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    double *outAroonDown = malloc(sizeof(double) * count);
    double *outAroonUp = malloc(sizeof(double) * count);
    ret = TA_AROON(0, count - 1, high, low, 14, &outBeg, &outNbElement, outAroonDown, outAroonUp);
    if (ret == TA_SUCCESS && outNbElement > 0) {
        out->aroon.aroon_down = outAroonDown[outNbElement - 1];
        out->aroon.aroon_up = outAroonUp[outNbElement - 1];
    } else {
        out->aroon.aroon_down = 0;
        out->aroon.aroon_up = 0;
    }
    free(outAroonDown);
    free(outAroonUp);

    // AROONOSC
    ret = TA_AROONOSC(0, count - 1, high, low, 14, &outBeg, &outNbElement, outReal);
    out->aroonosc = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    // BOP
    ret = TA_BOP(0, count - 1, open, high, low, close, &outBeg, &outNbElement, outReal);
    out->bop = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    // CCI 14
    ret = TA_CCI(0, count - 1, high, low, close, 14, &outBeg, &outNbElement, outReal);
    out->cci_14 = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_CMO(0, count - 1, close, 14, &outBeg, &outNbElement, outReal);
    out->cmo_14 = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_DX(0, count - 1, high, low, close, 14, &outBeg, &outNbElement, outReal);
    out->dx_14 = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_MFI(0, count - 1, high, low, close, volume, 14, &outBeg, &outNbElement, outReal);
    out->mfi_14 = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_MINUS_DI(0, count - 1, high, low, close, 14, &outBeg, &outNbElement, outReal);
    out->minus_di = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_MINUS_DM(0, count - 1, high, low, 14, &outBeg, &outNbElement, outReal);
    out->minus_dm = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_MOM(0, count - 1, close, 10, &outBeg, &outNbElement, outReal);
    out->mom_10 = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_PLUS_DI(0, count - 1, high, low, close, 14, &outBeg, &outNbElement, outReal);
    out->plus_di = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_PLUS_DM(0, count - 1, high, low, 14, &outBeg, &outNbElement, outReal);
    out->plus_dm = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_PPO(0, count - 1, close, 12, 26, TA_MAType_SMA, &outBeg, &outNbElement, outReal);
    out->ppo = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_ROC(0, count - 1, close, 10, &outBeg, &outNbElement, outReal);
    out->roc_10 = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_TRIX(0, count - 1, close, 30, &outBeg, &outNbElement, outReal);
    out->trix = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_ULTOSC(0, count - 1, high, low, close, 7, 14, 28, &outBeg, &outNbElement, outReal);
    out->ultosc = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_WILLR(0, count - 1, high, low, close, 14, &outBeg, &outNbElement, outReal);
    out->willr_14 = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    double *outMACD = malloc(sizeof(double) * count);
    double *outMACDSignal = malloc(sizeof(double) * count);
    double *outMACDHist = malloc(sizeof(double) * count);
    ret = TA_MACD(0, count - 1, close, 12, 26, 9, &outBeg, &outNbElement, outMACD, outMACDSignal, outMACDHist);
    if (ret == TA_SUCCESS && outNbElement > 0) {
        out->macd.macd = outMACD[outNbElement - 1];
        out->macd.signal = outMACDSignal[outNbElement - 1];
        out->macd.hist = outMACDHist[outNbElement - 1];
    } else {
        out->macd.macd = 0; out->macd.signal = 0; out->macd.hist = 0;
    }
    free(outMACD); free(outMACDSignal); free(outMACDHist);

    double *outSlowK = malloc(sizeof(double) * count);
    double *outSlowD = malloc(sizeof(double) * count);
    ret = TA_STOCH(0, count - 1, high, low, close, 5, 3, TA_MAType_SMA, 3, TA_MAType_SMA, &outBeg, &outNbElement, outSlowK, outSlowD);
    if (ret == TA_SUCCESS && outNbElement > 0) {
        out->stoch.k = outSlowK[outNbElement - 1];
        out->stoch.d = outSlowD[outNbElement - 1];
    } else {
        out->stoch.k = 0; out->stoch.d = 0;
    }
    
    double *outFastK = malloc(sizeof(double) * count);
    double *outFastD = malloc(sizeof(double) * count);
    ret = TA_STOCHF(0, count - 1, high, low, close, 5, 3, TA_MAType_SMA, &outBeg, &outNbElement, outFastK, outFastD);
    if (ret == TA_SUCCESS && outNbElement > 0) {
        out->stochf.k = outFastK[outNbElement - 1];
        out->stochf.d = outFastD[outNbElement - 1];
    } else {
        out->stochf.k = 0; out->stochf.d = 0;
    }
    free(outFastK); free(outFastD);

    ret = TA_STOCHRSI(0, count - 1, close, 14, 5, 3, TA_MAType_SMA, &outBeg, &outNbElement, outSlowK, outSlowD);
    if (ret == TA_SUCCESS && outNbElement > 0) {
        out->stochrsi.k = outSlowK[outNbElement - 1];
        out->stochrsi.d = outSlowD[outNbElement - 1];
    } else {
        out->stochrsi.k = 0; out->stochrsi.d = 0;
    }
    free(outSlowK); free(outSlowD);

    ret = TA_OBV(0, count - 1, close, volume, &outBeg, &outNbElement, outReal);
    out->obv = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_AD(0, count - 1, high, low, close, volume, &outBeg, &outNbElement, outReal);
    out->ad = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_ADOSC(0, count - 1, high, low, close, volume, 3, 10, &outBeg, &outNbElement, outReal);
    out->adosc = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    double *outUpper = malloc(sizeof(double) * count);
    double *outMiddle = malloc(sizeof(double) * count);
    double *outLower = malloc(sizeof(double) * count);
    ret = TA_BBANDS(0, count - 1, close, 20, 2.0, 2.0, TA_MAType_SMA, &outBeg, &outNbElement, outUpper, outMiddle, outLower);
    if (ret == TA_SUCCESS && outNbElement > 0) {
        out->bbands.upper = outUpper[outNbElement - 1];
        out->bbands.middle = outMiddle[outNbElement - 1];
        out->bbands.lower = outLower[outNbElement - 1];
    } else {
        out->bbands.upper = 0; out->bbands.middle = 0; out->bbands.lower = 0;
    }
    free(outUpper); free(outMiddle); free(outLower);

    ret = TA_ATR(0, count - 1, high, low, close, 14, &outBeg, &outNbElement, outReal);
    out->atr_14 = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_NATR(0, count - 1, high, low, close, 14, &outBeg, &outNbElement, outReal);
    out->natr_14 = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_TRANGE(0, count - 1, high, low, close, &outBeg, &outNbElement, outReal);
    out->trange = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_HT_DCPERIOD(0, count - 1, close, &outBeg, &outNbElement, outReal);
    out->ht_dcperiod = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    ret = TA_HT_DCPHASE(0, count - 1, close, &outBeg, &outNbElement, outReal);
    out->ht_dcphase = (ret == TA_SUCCESS && outNbElement > 0) ? outReal[outNbElement - 1] : 0;

    int *outInteger = malloc(sizeof(int) * count);
    ret = TA_HT_TRENDMODE(0, count - 1, close, &outBeg, &outNbElement, outInteger);
    out->ht_trendmode = (ret == TA_SUCCESS && outNbElement > 0) ? (double)outInteger[outNbElement - 1] : 0;
    free(outInteger);

    double *outInPhase = malloc(sizeof(double) * count);
    double *outQuadrature = malloc(sizeof(double) * count);
    ret = TA_HT_PHASOR(0, count - 1, close, &outBeg, &outNbElement, outInPhase, outQuadrature);
    if (ret == TA_SUCCESS && outNbElement > 0) {
        out->ht_phasor.inphase = outInPhase[outNbElement - 1];
        out->ht_phasor.quadrature = outQuadrature[outNbElement - 1];
    } else {
        out->ht_phasor.inphase = 0; out->ht_phasor.quadrature = 0;
    }
    free(outInPhase); free(outQuadrature);

    double *outSine = malloc(sizeof(double) * count);
    double *outLeadSine = malloc(sizeof(double) * count);
    ret = TA_HT_SINE(0, count - 1, close, &outBeg, &outNbElement, outSine, outLeadSine);
    if (ret == TA_SUCCESS && outNbElement > 0) {
        out->ht_sine.sine = outSine[outNbElement - 1];
        out->ht_sine.leadsine = outLeadSine[outNbElement - 1];
    } else {
        out->ht_sine.sine = 0; out->ht_sine.leadsine = 0;
    }
    free(outSine); free(outLeadSine);

    int *outInt = malloc(sizeof(int) * count);
    
    ret = TA_CDLDOJI(0, count - 1, open, high, low, close, &outBeg, &outNbElement, outInt);
    out->cdl_doji = (ret == TA_SUCCESS && outNbElement > 0) ? outInt[outNbElement - 1] : 0;

    ret = TA_CDLHAMMER(0, count - 1, open, high, low, close, &outBeg, &outNbElement, outInt);
    out->cdl_hammer = (ret == TA_SUCCESS && outNbElement > 0) ? outInt[outNbElement - 1] : 0;

    ret = TA_CDLENGULFING(0, count - 1, open, high, low, close, &outBeg, &outNbElement, outInt);
    out->cdl_engulfing = (ret == TA_SUCCESS && outNbElement > 0) ? outInt[outNbElement - 1] : 0;

    ret = TA_CDLMORNINGSTAR(0, count - 1, open, high, low, close, 0.3, &outBeg, &outNbElement, outInt);
    out->cdl_morningstar = (ret == TA_SUCCESS && outNbElement > 0) ? outInt[outNbElement - 1] : 0;

    ret = TA_CDLEVENINGSTAR(0, count - 1, open, high, low, close, 0.3, &outBeg, &outNbElement, outInt);
    out->cdl_eveningstar = (ret == TA_SUCCESS && outNbElement > 0) ? outInt[outNbElement - 1] : 0;

    ret = TA_CDL3BLACKCROWS(0, count - 1, open, high, low, close, &outBeg, &outNbElement, outInt);
    out->cdl_3blackcrows = (ret == TA_SUCCESS && outNbElement > 0) ? outInt[outNbElement - 1] : 0;

    ret = TA_CDL3WHITESOLDIERS(0, count - 1, open, high, low, close, &outBeg, &outNbElement, outInt);
    out->cdl_3whitesoldiers = (ret == TA_SUCCESS && outNbElement > 0) ? outInt[outNbElement - 1] : 0;

    free(outInt);
    free(outReal);

    out->valid = 1;

    free(open);
    free(high);
    free(low);
    free(close);
    free(volume);
}
