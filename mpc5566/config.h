#ifndef __CONFIG_H__
#define __CONFIG_H__

// Where the exception table should be installed
#define tablebase 0x40000000

// Where the main image should be extracted to (and ran from)
#define mainbase  0x4000C000


#define CANBOX_RX   ( 0 )
#define CANBOX_TX   ( 1 )

#ifdef enableDebugBOX
#define CANBOX_DBG  ( 2 )
#ifdef enableBroadcast
#define CANBOX_BRC  ( 3 )
#endif
#else
#ifdef enableBroadcast
#define CANBOX_BRC  ( 2 )
#endif
#endif

#define localID     0x7e8
#define hostID      0x7e0
#define broadcastID 0x101
#define debugID     0x111

#endif
