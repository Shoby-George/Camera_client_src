#ifndef CLIENT_H
#define CLIENT_H

#include "circfifo.h"

#define DISCONNECTED                    0
#define READY                           2
#define CONNECTED                       1
#define CLOSE                           3
#define INVALID                         4

#define MAX_NO_SOCKETS                  8

#define SEQUENCE_NO                     3
#define PACKET_LENGTH                   4

#define REMOTE_KEEPALIVETIMEOUT         28

#define SOCKET_SEND_BUFF_LENGTH         (1024 * 4)

#define MAXREMOTE_RX_DATA_FIFO_LENGTH   (512*1024)
#define MAXREMOTE_TX_DATA_FIFO_LENGTH   (16*1024)

#define POLL_TIMEOUT_MSEC               30
#define MAX_SOCKET_DATA_TR              SOCKET_SEND_BUFF_LENGTH

#define MAX_TX_DATA_BUFFER              (16*1024)
#define MAX_RX_DATA_BUFFER              (512*1024)

#define MAX_TX_DATA_SIZE_MASK           (MAX_TX_DATA_BUFFER + (MAX_TX_DATA_BUFFER-1)) ^ (MAX_SOCKET_DATA_TR-1)
#define MAX_WDT_COUNT                   2
#define NO_OF_PROCESS                   2
#define ONE_SEC_TIMER                   250

#define HEADER1                         0x5432;
#define HEADER2                         0x9876;
#define MAX_CLIENT_CONNECTIONS          4

// ---------------------- structure defines --------------------
struct tst {
    unsigned int sock_number;                       // connection number for tracing..
    unsigned int connection_mode;                   // Server=0/Client=1
    unsigned int connection_status;                 // READY=2/CONNECTED=1/DISCONNECTED=0
    char ip[18];                                    // ip no
    char port_no[5];                                // port no
    int strm;                                       // stream, 0 := HD, 1 := VGA
    unsigned int local_host_keep_alive;             // local keep alive
    unsigned int client_keep_alive_tx_count;        // remote keep alive tx delay.
    unsigned int client_keep_alive_tx_status;       // remote keep alive tx delay.
    unsigned int remote_keep_alive_count;           // remote keep alive status
    unsigned int remote_keep_alive_count_reload;    // remote keep alive reload value
    unsigned int send_communication_init;           // start comm command 0x8A..
    int socket_handle;                              // handle...
    struct cirfifo        *remote_tx_fifo;          //
    struct cirfifo        *remote_rx_fifo;          //
    unsigned short        *rx_data_buffer;
    unsigned short        *tx_data_buffer;
    unsigned long expected_len;
    unsigned int rx_in_progress;
    unsigned short sequence_no;
};

struct wdt {
    unsigned int enabled;
    unsigned int res1;
    unsigned int res2;
    unsigned int res3;
    unsigned int *keep_alive_timer;
    unsigned int *keep_alive_reload;
    unsigned int *aux_keep_alive_timer;
    unsigned int *err;
};

struct ipconfigs
{
    int valid, strm;
    char ipaddr[18];
    char port[5];
};

#pragma pack(1)
struct E2V_RED_CAPTURE_CMD
{
    unsigned short header;/* headerStartMarker is 0xA235 */
    unsigned char destination;
    unsigned char cmd;
    /*see typedef*/
    unsigned char flags;
    /*see typedef*/
    unsigned char FLX_cordinate;
    unsigned char FLY_cordinate;
    unsigned char FRX_cordinate;
    unsigned char FRY_cordinate;
    uint32_t violationNumber;       /*violation number for this capture*/
    uint32_t xFpsSequenceNumber;/*xFpsCapture number, ANPR NVR capture-sequence number for this capture*/
    unsigned char vehID;// vehicle number in image.
    unsigned char RLX_cordinate;
    unsigned char RLY_cordinate;
    unsigned char RRX_cordinate;
    unsigned char RRY_cordinate;
    unsigned char veh_type;
    unsigned char reserved;
    /*reserved for debugging*/
} T_E2V_RED_CAPTURE_CMD;
#pragma pack(0)

struct img_header {// use this with only version no > F152_036. no backward compatiblity.
    unsigned short header1;
    unsigned short header2;
    unsigned long img_size;
    unsigned short offset;// no of header bytes to skip from file start..(32)
    unsigned short width;
    unsigned short height;
    unsigned char type;// 0 = color /1 = monochrome
    unsigned char exposure_type;//  0 = high/normal, 1= low;
    unsigned long sequence_no;//  capture count; for night capture, high and low will have same seq no..
    unsigned char flash_status;// flash on / off 0=off(single capture), 1=on(double capture).. on transition boundary it is not synced. (USE WITH exposure type)
    unsigned char avg_light_value;// average lux value(255-1)
    unsigned char light_table_index;//0-31 fore noon, 32-63 afternoon, 64 night.
    unsigned char cnt_sec;// 0-fps// count of the image captured in this second
    unsigned char sec;// 0-59
    unsigned char min;// 0-59
    unsigned char hour;// 0-24
    unsigned char day;// 1-28,30,31
    unsigned char month;// 0-11
    unsigned char year;// 1900 + value
    unsigned char lane_evidence;
    unsigned char lane_no;
    unsigned char no_of_vehicles;
    unsigned char vehicle_type[8];
    unsigned long lba;
};
#endif

