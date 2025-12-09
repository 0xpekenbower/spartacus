import WebSocket from 'ws';
import { EventEmitter } from 'events';
import { logger } from '../utils/logger.js';

class BinanceFuturesWS extends EventEmitter {
  constructor() {
    super();
    this.baseUrl = 'wss://fstream.binance.com/ws';
    this.ws = null;
    this.pingInterval = null;
    this.subscriptions = new Set();
  }

  connect(symbols, timeframes) {
    this.ws = new WebSocket(this.baseUrl);

    this.ws.on('open', () => {
      logger.info('Connected to Binance Futures WebSocket');
      this.subscribe(symbols, timeframes);
    });

    this.ws.on('message', (data) => {
      try {
        const message = JSON.parse(data);
        
        if (message.e === 'kline') {
          this.handleKline(message);
        }
      } catch (error) {
        logger.error('Error parsing message:', error);
      }
    });

    this.ws.on('close', () => {
      logger.warn('WebSocket connection closed. Reconnecting in 5s...');
      setTimeout(() => this.connect(symbols, timeframes), 5000);
    });

    this.ws.on('error', (error) => {
      logger.error('WebSocket error:', error);
    });
  }

  subscribe(symbols, timeframes) {
    const params = [];
    
    symbols.forEach(symbol => {
      const formattedSymbol = symbol.replace('/', '').toLowerCase();
      timeframes.forEach(tf => {
        const streamName = `${formattedSymbol}@kline_${tf}`;
        params.push(streamName);
        this.subscriptions.add(streamName);
      });
    });

    if (params.length > 0) {
      const payload = {
        method: 'SUBSCRIBE',
        params: params,
        id: Date.now()
      };
      
      logger.info(`Subscribing to streams: ${params.join(', ')}`);
      this.ws.send(JSON.stringify(payload));
    }
  }

  handleKline(message) {
    const k = message.k;

    if (!k.x) {
      return;
    }

    const output = {
      t: k.t,
      T: k.T,
      s: k.s,
      i: k.i,
      o: k.o,
      c: k.c,
      h: k.h,
      l: k.l,
      v: k.v 
    };
    this.emit('candle', output);
  }
}
export const binanceWs = new BinanceFuturesWS();
