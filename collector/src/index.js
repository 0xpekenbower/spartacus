import { binanceWs } from './exchanges/binanceWs.js';
import { pairConfig } from './config/pairConfig.js';
import { logger } from './utils/logger.js';

async function run() {
  try {
    logger.info('Starting Collector Service (WebSocket Mode)...');
    const { symbols, timeframes } = pairConfig;
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
