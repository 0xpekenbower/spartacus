#ifndef PAIRS_H
#define PAIRS_H

// Fetches top pairs from Binance and updates the global app_config.json object
// Filters: USDT pairs, Price > 5, Top 50 by 24h Change %
int update_config_with_top_pairs();

#endif
