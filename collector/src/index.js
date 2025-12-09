import { binanceWs } from './exchanges/binanceWs.js';
import { pairConfig } from './config/pairConfig.js';
import { logger } from './utils/logger.js';
import { calculationClient } from './services/calculationClient.js';

async function run() {
  try {
    logger.info('Starting Collector Service (WebSocket Mode)...');    
    calculationClient.connect();
    const { symbols, timeframes } = pairConfig;
    binanceWs.on('candle', (candle) => {
      logger.info(`Received candle for ${candle.s}, sending to Calculation Service`);
      calculationClient.send(candle);
    });

    binanceWs.connect(symbols, timeframes);
  } catch (error) {
    logger.error('Critical error in main loop:', error);
  }
}

run();

process.on('SIGINT', () => {
  logger.info('Shutting down...');
  process.exit(0);
});
