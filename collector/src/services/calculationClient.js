import WebSocket from 'ws';
import { logger } from '../utils/logger.js';

class CalculationClient {
  constructor(url) {
    this.url = url;
    this.ws = null;
    this.isConnected = false;
    this.queue = [];
  }

  connect() {
    if (!this.url) {
      logger.warn('CALCULATION_URL not set, skipping connection to Calculation Service');
      return;
    }

    this.ws = new WebSocket(this.url);

    this.ws.on('open', () => {
      logger.info('Connected to Calculation Service');
      this.isConnected = true;
      this.flushQueue();
    });

    this.ws.on('close', () => {
      logger.warn('Disconnected from Calculation Service. Reconnecting in 5s...');
      this.isConnected = false;
      setTimeout(() => this.connect(), 5000);
    });

    this.ws.on('error', (error) => {
      logger.error('Calculation Service WebSocket error:', error);
    });
  }

  send(data) {
    if (this.isConnected && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify(data));
    } else {
      // Optional: Queue messages if disconnected, but for real-time candles, maybe just drop or queue limited amount
      // For now, let's just log a warning or drop it to avoid memory leaks if down for long
      // logger.warn('Calculation Service not connected, dropping message');
    }
  }

  flushQueue() {
    // Implement if queueing is needed
  }
}

export const calculationClient = new CalculationClient(process.env.CALCULATION_URL);
