#ifndef __CONFIG_H__
#define __CONFIG_H__

// Where the exception table should be installed
#define tablebase 0x40000000

#define CANBOX_RX   ( 0 )
#define CANBOX_TX   ( 1 )

#if (defined(enableDebugBox) && defined(enableBroadcast))
#define CANBOX_DBG  ( 2 )
#define CANBOX_BRC  ( 3 )
#elif defined(enableDebugBox)
#define CANBOX_DBG  ( 2 )
#elif defined(enableBroadcast)
#define CANBOX_BRC  ( 2 )
#endif

#define localID     0x7e8
#define hostID      0x7e0
#define broadcastID 0x101
#define debugID     0x111

#endif
