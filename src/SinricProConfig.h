#ifndef __SINRICPRO_CONFIG_H__
#define __SINRICPRO_CONFIG_H__

#define SERVER_URL "23.95.122.232" 
#define SERVER_PORT 3001

#define UDP_MULTICAST_IP IPAddress(224,9,9,9)
#define UDP_MULTICAST_PORT 3333

// if websocket connection cannot established after WEBSOCKET_CONNECTION_TIMEOUT milliseconds
// websocket will try to connect again
#define WEBSOCKET_CONNECTION_TIMEOUT 30000

// websocket sends every WEBSOCKET_PING_INTERVAL milliseconds a ping to Server
// if there is no pong received after WEBSOCKET_PING_TIMEOUT milliseconds, retry count is incremented by one
// if retry count reaches WEBSOCKET_RETRY_COUNT websocket connection is closed and try to reconnect server
#define WEBSOCKET_PING_INTERVAL 30000
#define WEBSOCKET_PING_TIMEOUT 10000
#define WEBSOCKET_RETRY_COUNT 2

#endif
