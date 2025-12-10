#ifndef TYPES_H
#define TYPES_H

#include <stddef.h>

typedef struct {
    double t;
    double T;
    char s[16];
    char i[4];
    double o;
    double c;
    double h;
    double l;
    double v;
} Candle;

typedef struct {
    double ema_14;
    double sma_14;
    double wma_14;
    double kama_14;
    double tema_14;
    double trima_14;
    double sar;
    
    double rsi_14;
    struct {
        double macd;
        double signal;
        double hist;
    } macd;
    double adx_14;
    double adxr_14;
    double apo;
    struct {
        double aroon_down;
        double aroon_up;
    } aroon;
    double aroonosc;
    double bop;
    double cci_14;
    double cmo_14;
    double dx_14;
    double mfi_14;
    double minus_di;
    double minus_dm;
    double mom_10;
    double plus_di;
    double plus_dm;
    double ppo;
    double roc_10;
    struct {
        double k;
        double d;
    } stoch;
    struct {
        double k;
        double d;
    } stochf;
    struct {
        double k;
        double d;
    } stochrsi;
    double trix;
    double ultosc;
    double willr_14;

    double obv;
    double ad;
    double adosc;

    struct {
        double upper;
        double middle;
        double lower;
    } bbands;
    double atr_14;
    double natr_14;
    double trange;

    double ht_dcperiod;
    double ht_dcphase;
    struct {
        double inphase;
        double quadrature;
    } ht_phasor;
    struct {
        double sine;
        double leadsine;
    } ht_sine;
    double ht_trendmode;

    int cdl_doji;
    int cdl_hammer;
    int cdl_engulfing;
    int cdl_morningstar;
    int cdl_eveningstar;
    int cdl_3blackcrows;
    int cdl_3whitesoldiers;

    int valid;
} Indicators;

#endif
