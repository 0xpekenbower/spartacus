import { WebSocketServer } from 'ws';

const PORT = process.env.PORT || 8080;
const wss = new WebSocketServer({ port: PORT });

console.log(`Calculation Service started on port ${PORT}`);

wss.on('connection', (ws) => {
  console.log('Collector connected');

  ws.on('message', (data) => {
    try {
      const candle = JSON.parse(data);
      console.log('Received candle:', candle);
      // TODO: Process candle
    } catch (error) {
      console.error('Error parsing message:', error);
    }
  });

  ws.on('close', () => {
    console.log('Collector disconnected');
  });

  ws.on('error', (error) => {
    console.error('WebSocket error:', error);
  });
});
