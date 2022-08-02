/* 
 * *********************************************************************
 *		Project		:	RX Camera Stream
 *		Filename	:	threadedCleintSocket.c
 *		Function	:	Process Main Routines and INIT
 *		Author		:	Manoj Kumar D
 *		Mediatronix Pvt Ltd,Pappanamcode,Industrial Estate,Trivandrum.
 *
 ** spawn a thread for every client.
 ** handle all data in main process.
 ** use poll with timeout 20ms or more..
 * *********************************************************************
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/sockios.h>
#include <assert.h>

#include "config_parser.h"
#include "helpers.h"

#define MAX_ATTEMPTS 10


static struct sigaction sa;
static struct itimerval timer;
static struct wdt keepalive_timer[MAX_CLIENT_CONNECTIONS];

static unsigned int app_exit = 0;
static char *img_dir = NULL; 

int server_tx(struct tst *tst);
void server_rx(struct tst *tst);
void process_tx(struct tst *tst);
void process_rx(struct tst *tst);
void copy_status_flags_n_send(unsigned int tx_size, struct tst *tst);
void threadedSocketThread(void * arg);
void kill_timer();
void init_timer();

int main(int argc, char* argv[])
{
    int i, j, opt;
    struct config *cfg;
    struct stat st = {0};

    void *ret;  // For joining threads
    int p_ret[MAX_CLIENT_CONNECTIONS];
    int total_spawned_threads = 0;

    char filename[PATH_MAX] = "config.json";

    char *usage_string =
        "Usage:\n%s [-c config-file] [-h]\n\n"
        "All arguments are optional.\n"
        "   -c config-file\t:\ta JSON configuration file. Default: ./config.json\n"
        "   -h            \t:\tthis help/usage messase\n";

    opterr = 0;

    while ((opt = getopt(argc, argv, "c:h")) != -1) {
        switch (opt) {
            case 'c':
                if (optarg != NULL)
                    strcpy(filename, optarg);
                break;
            case 'h':
                printf(usage_string, argv[0]);
                exit(EXIT_SUCCESS);
                break;
            default:
                fprintf(stderr, "Error parsing arguments. Unknown argument: '%c'\n", opt);
                fprintf(stderr, usage_string, argv[0]);
                exit(EXIT_FAILURE);
                break;
        }
    }

    if (access(filename, F_OK) == -1) {
        fprintf(stderr, "Config file does not exist: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    printf("Reading configuration from %s\n", filename);
    cfg = config_parser(filename);
    img_dir = cfg->img_dir;
    assert(cfg->n_config <= MAX_CLIENT_CONNECTIONS);

    for (i = 0; i < MAX_CLIENT_CONNECTIONS; i++) {
        p_ret[i] = 0;
    }

    /* Check if the img_dir exists or not: if not try to create one. */
    if (stat(img_dir, &st) == -1) {
        fprintf(stderr, "\t--\n\t-- %s : %s\n", strerror(errno), img_dir);
        fprintf(stderr, "\t-- Creating image directory : %s\n\t--\n", img_dir);
        mkdir(img_dir, 0755);
    }

    static pthread_t sock_thread[MAX_CLIENT_CONNECTIONS];
    static struct tst sock_thread_data[MAX_CLIENT_CONNECTIONS];
    struct ipconfigs *ipconfig = cfg->configs;

    j = 0;

    printf("start main init\n");
    for(i = 0; i < cfg->n_config; i++)
        keepalive_timer[i].enabled = 0;
    init_timer();
    printf("main init over\n");

    for(i = 0; i < cfg->n_config; i++)
    {
        if(ipconfig[i].valid)
        {
            sprintf(sock_thread_data[i].ip, "%s", ipconfig[i].ipaddr);
            sprintf(sock_thread_data[i].port_no, "%s", ipconfig[i].port);
            sock_thread_data[i].strm = ipconfig[i].strm;
            sock_thread_data[i].sock_number = i;
            sock_thread_data[i].connection_status = DISCONNECTED;
            sock_thread_data[i].remote_keep_alive_count = 0;
            sock_thread_data[i].client_keep_alive_tx_count = 0;
            keepalive_timer[i].keep_alive_timer = &sock_thread_data[i].remote_keep_alive_count;
            keepalive_timer[i].keep_alive_reload = &sock_thread_data[i].remote_keep_alive_count_reload;
            keepalive_timer[i].aux_keep_alive_timer = &sock_thread_data[i].client_keep_alive_tx_count;
            keepalive_timer[i].enabled = 1;
            printf("thread %d create\n", i);
            pthread_create(&sock_thread[i], NULL, (void *) &threadedSocketThread, (void *) &(sock_thread_data[i]));
            printf("thread %d created\n", i);
            j++; // j here is the number of threads spawned.
        }
    }

    // process loop...
    if(j > 0) // if any sockets configured..
    {
        while(1)
        {
            // check for images and handle it here....
            // check for valid or configured threads and handle if any error..
            usleep(50000); // 50ms -- make this delay as small as possible while processing images

            // Check which threads exited, then join them. If all
            // threads exited, then join all of them and exit the process.
            /*for (i = 0; i < cfg->n_config; i++) {
                if (ipconfig[i].valid) {
                    if (pthread_join(sock_thread[i], &ret) == 0)
                        p_ret[i]++;
                }
            }

            for (i = 0; i < cfg->n_config; i++)
                total_spawned_threads += p_ret[i];

            if (total_spawned_threads >= j)
                break;
            */
        }
    }

    /* if it reaches here means that there is a problem -- most likely
     * camera is not available on the given ip:port */
    // return EXIT_FAILURE;
}

int image_save(unsigned long size, unsigned char* img_buff, unsigned char *metadata, unsigned char *seq_cap_data)       // process recieved image ---
{
    FILE *fptr;
    char filename[128];
    static unsigned char header_data[sizeof(struct img_header)] = {0x5A, 0xC3, 0xA5, 0x0F, 0, 0, 0, 0, 32, 0, 5, 0xC0, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};// same as img_header structure
    int i, j;
    unsigned long sequence_no, img_cnt;
    unsigned char flash_status, avg_light_value, light_table_index, sec, min, hour, day, month, year, cnt_sec, exposure_type;
    struct E2V_RED_CAPTURE_CMD *capdata;
    int valid = 0;
    int channel = 0x0F;
    unsigned char lane_no, lane_evidence, no_of_vehicles = 0, vehicle_type[8] = {0};
    struct img_header *img_header_ptr;

    img_header_ptr = (struct img_header *) header_data;
    capdata = (struct E2V_RED_CAPTURE_CMD *)seq_cap_data;
    /*printf("capture data\n");
      display_buffer(seq_cap_data, 192);
      printf("Header data\n");
      display_buffer(metadata, 128);*/

    lane_no = metadata[22];// lane no..
    lane_evidence = metadata[23];// evidence/lane
    sequence_no = 0;
    sequence_no = metadata[118];
    sequence_no |= metadata[119] << 8;
    sequence_no |= metadata[120] << 16;
    sequence_no |= metadata[121] << 24;

    avg_light_value = metadata[127];
    light_table_index = metadata[126];

    flash_status = metadata[46];
    exposure_type = metadata[44];
    cnt_sec = metadata[42];
    sec = metadata[40];
    min = metadata[38];
    hour = metadata[36];
    day = metadata[34];
    month = metadata[32];
    year = metadata[30];

    j = 0;
    for(i = 1; i< 64; i++)
    {
        if(img_buff[size - i] == 0xd9)
        {
            if(img_buff[size - (i+1)] == 0xFF)
            {
                size = (size+1) - i;
                j = 1;
                break;
            }
        }
    }

    if(metadata[22])
        channel = 0;
    else
        channel = metadata[23]+1;

    img_header_ptr->img_size            = size;
    img_header_ptr->offset              = sizeof(struct img_header); // no of header bytes to skip from file start..(32)
    img_header_ptr->width               = 1280;
    img_header_ptr->height              = 960;
    img_header_ptr->type                = 0; // 0 = color /1 = monochrome
    img_header_ptr->exposure_type       = exposure_type;//  0 = high/normal, 1= low;
    img_header_ptr->sequence_no         = sequence_no;//  capture count; for night capture, high and low will have same seq no..
    img_header_ptr->flash_status        = flash_status;// flash on/off 0=off (single capture), 1=on (double capture). on transition boundary it is not synced. (USE WITH exposure type)
    img_header_ptr->avg_light_value     = avg_light_value;// average lux value(255-1)
    img_header_ptr->light_table_index   = light_table_index;//0-31 fore noon, 32-63 afternoon, 64 night.
    img_header_ptr->cnt_sec             = cnt_sec;// 0-fps// count of the image captured in this second
    img_header_ptr->sec                 = sec;// 0-59
    img_header_ptr->min                 = min;// 0-59
    img_header_ptr->hour                = hour;// 0-24
    img_header_ptr->day                 = day;// 1-28,30,31
    img_header_ptr->month               = month;// 0-11
    img_header_ptr->year                = year;// 1900 + value
    img_header_ptr->lane_evidence       = lane_evidence;// 0=lane, 1=evidence
    img_header_ptr->lane_no             = lane_no;// 0=lane1, 1=lane2, etc...
    img_header_ptr->no_of_vehicles      = no_of_vehicles;

    if (!(metadata[106] & 7)) {
        if(exposure_type) //flash second image
            sprintf(filename, "%s/%04d-%02d-%02dT%02d:%02d:%02d_%02d_%s_H%02d.jpg", img_dir, 1900+year, 1+month, day, hour, min, sec, cnt_sec, (lane_evidence) ? "EVID" : "ANPR", lane_no);
        else //flash first image
            sprintf(filename, "%s/%04d-%02d-%02dT%02d:%02d:%02d_%02d_%s_L%02d.jpg", img_dir, 1900+year, 1+month, day, hour, min, sec, cnt_sec, (lane_evidence) ? "EVID" : "ANPR", lane_no);
    } else {
        if(exposure_type) //flash second image
            sprintf(filename, "%s/%04d-%02d-%02dT%02d:%02d:%02d_%02d_%s_H%02d.jpg", img_dir, 1900+year, 1+month, day, hour, min, sec, cnt_sec, "VGA", lane_no);
        else //flash first image
            sprintf(filename, "%s/%04d-%02d-%02dT%02d:%02d:%02d_%02d_%s_L%02d.jpg", img_dir, 1900+year, 1+month, day, hour, min, sec, cnt_sec, "VGA", lane_no);

    }

    // ===================================================================
    if (img_cnt % 30 == 0)
        printf("Saving %s with CaptureSeqNo %ld of Length %ld time stamp %d-%d-%d %d:%d:%d.%d, lux=%d, table=%d, lane_evidence %d, lane_no %d\n", 
                filename, 
                sequence_no, 
                size, 
                year+1900, month+1, day, hour, min, sec, cnt_sec, 
                avg_light_value, 
                light_table_index, 
                lane_evidence, 
                lane_no);
    // ===================================================================
    fptr = fopen(filename, "wb");
    if(fptr != NULL) 
    {
        fwrite(img_buff, 1, size, fptr);
        fclose(fptr);
    }

    img_cnt++;
}

// kill >>>>>>>>>>>>>>>>>>>
void kill_timer()
{
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0; /* ... now.... */
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;  /* here...*/
    setitimer (ITIMER_REAL, &timer, NULL);
}

void timer_handler(int signum)
{
    static int sec_timer;
    int rtemp;
    unsigned int *timer_var, *timer_reload, *auxtimer;

    if(sec_timer == 0)
    {
        sec_timer = ONE_SEC_TIMER - 1;
        for(rtemp = 0; rtemp < MAX_CLIENT_CONNECTIONS; rtemp++)
        {
            if(keepalive_timer[rtemp].enabled)
            {
                timer_var = keepalive_timer[rtemp].keep_alive_timer;
                timer_reload = keepalive_timer[rtemp].keep_alive_reload;
                auxtimer = keepalive_timer[rtemp].aux_keep_alive_timer;
                if(*timer_var > 0)
                {
                    if(*timer_reload > 0)
                    {
                        *timer_var = *timer_reload;
                        *timer_reload = 0;
                    }
                    else
                    {
                        *timer_var = (*timer_var) -1;
                    }
                }
                if((*auxtimer) > 0)
                    *auxtimer = (*auxtimer)-1;
            }
        }
    }
    else
        sec_timer--;
}

void init_timer()
{
    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &timer_handler;
    sigaction (SIGALRM, &sa, NULL);

    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 4000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 4000;

    setitimer (ITIMER_REAL, &timer, NULL);  /* Start a virtual timer. It counts down whenever this process is executing. */
}

void threadedSocketThread(void * arg)
{
    struct tst                    *tst_var;
    char ipaddrs[18]={0};
    struct sockaddr_in socket_addr;
    socklen_t addr_len;
    int attempt = 0;
    unsigned char *remote_data_tx_fifo_buff;// memory for circular fifo
    unsigned char *remote_data_rx_fifo_buff;// memory for circular fifo
    unsigned short rx_data_buffer[MAX_RX_DATA_BUFFER], tx_data_buffer[MAX_TX_DATA_BUFFER];
    struct cirfifo remote_tx_fifo, remote_rx_fifo;
    struct pollfd socket_poll;
    unsigned short sequence_no = 0;
    int i, j;

    // init
    tst_var = (struct tst *)arg;
    printf("thread %d started init\n", tst_var->sock_number);
    remote_data_tx_fifo_buff = malloc(MAXREMOTE_TX_DATA_FIFO_LENGTH);
    remote_data_rx_fifo_buff = malloc(MAXREMOTE_RX_DATA_FIFO_LENGTH);
    tst_var->rx_data_buffer = rx_data_buffer;
    tst_var->tx_data_buffer = tx_data_buffer;
    tst_var->remote_tx_fifo = &remote_tx_fifo;
    tst_var->remote_rx_fifo = &remote_rx_fifo;

    printf("socket %d connection status = %d\n", tst_var->sock_number, tst_var->connection_status);

    // processing.. loop
    printf("socket %d thread_started looping\n", tst_var->sock_number);
    while(1)
    {
        //------------------------------------------------------
        if(tst_var->connection_status == DISCONNECTED)
        {
            // initialize all socket related parameters
            if(tst_var->client_keep_alive_tx_count == 0)
            {
                fifo_init(&remote_tx_fifo, MAXREMOTE_TX_DATA_FIFO_LENGTH, remote_data_tx_fifo_buff);
                fifo_init(&remote_rx_fifo, MAXREMOTE_RX_DATA_FIFO_LENGTH, remote_data_rx_fifo_buff);
                inet_aton(tst_var->ip,(struct in_addr *)(&socket_addr.sin_addr));
                socket_addr.sin_family = AF_INET;
                socket_addr.sin_port = htons(atoi(tst_var->port_no));
                addr_len = sizeof(struct sockaddr_in);
                tst_var->local_host_keep_alive = 2;
                tst_var->socket_handle = socket(PF_INET, SOCK_STREAM, 0);
                j = SOCKET_SEND_BUFF_LENGTH*2;
                i = setsockopt(tst_var->socket_handle, SOL_SOCKET, SO_SNDBUF, &j, sizeof(int));
                j = 1;
                i = setsockopt(tst_var->socket_handle, SOL_TCP, TCP_NODELAY, &j, sizeof(int));
                tst_var->connection_status = READY;
                tst_var->client_keep_alive_tx_count = 2;
                fcntl(tst_var->socket_handle, F_SETFL, O_NONBLOCK);
                attempt = 0;
            }
        }
        //------------------------------------------------------
        else if(tst_var->connection_status == READY)
        {
            // attempt to connect socket..
            if(tst_var->client_keep_alive_tx_count == 0)
            {
                attempt++;
                printf("sock %d thread attempt to connect %s on %s\n", tst_var->sock_number, tst_var->ip, tst_var->port_no);
                if(connect(tst_var->socket_handle,(struct sockaddr *)&socket_addr,addr_len) == 0)
                {
                    attempt = 0;
                    tst_var->connection_status = CONNECTED;
                    tst_var->client_keep_alive_tx_count = 4;
                    tst_var->client_keep_alive_tx_status = 1;
                    tst_var->sequence_no = 1;
                    tst_var->expected_len = 0xffff;
                    tst_var->rx_in_progress = 0;
                    tst_var->remote_keep_alive_count = REMOTE_KEEPALIVETIMEOUT;
                    tst_var->remote_keep_alive_count_reload = REMOTE_KEEPALIVETIMEOUT;
                    tst_var->send_communication_init = 1;
                    printf("sock %d thread connected\n",tst_var->sock_number);
                    usleep(200000);
                }
                else
                {
                    printf("sock %d thread connection attempt %d\n",tst_var->sock_number, attempt);
                    tst_var->client_keep_alive_tx_count = 2;
                }
            }

            if (attempt > MAX_ATTEMPTS)
                goto deinit;
        }
        //------------------------------------------------------
        else if(tst_var->connection_status == CONNECTED)
        {
            // poll the socket and do tx and rx, if it fails disconnect.
            if(tst_var->remote_keep_alive_count == 0)
            {
                printf("sock %d keep alive timout socket.. disconnecting...\n", tst_var->sock_number);
                tst_var->connection_status = CLOSE;
            }
            else if(tst_var->remote_keep_alive_count >= 100)
            {
                printf("sock %d timer error or data couroupted.. disconnect and exit.....\n", tst_var->sock_number);
                app_exit = 1;
                break;
            }
            else
            {
                socket_poll.fd = tst_var->socket_handle;
                socket_poll.events = POLLIN|POLLOUT|POLLHUP|POLLNVAL|POLLERR;
                if(poll(&socket_poll, 1, POLL_TIMEOUT_MSEC) > 0)
                {
                    if((socket_poll.revents & POLLHUP) || (socket_poll.revents & POLLNVAL) || (socket_poll.revents & POLLERR))
                    {
                        printf("sock %d socket err remote hungup\n", tst_var->sock_number);
                        tst_var->connection_status = CLOSE;
                    }
                    else if(socket_poll.revents & POLLOUT)
                    {
                        if(server_tx(tst_var) == 0)
                        {
                            if(socket_poll.revents & POLLIN)
                            {
                                server_rx(tst_var);
                                process_rx(tst_var);
                            }
                            else
                            {
                                process_rx(tst_var);
                                process_tx(tst_var);
                                usleep(10000);
                            }
                        }
                        else
                        {
                            usleep(10000);
                        }
                    }
                    else
                    {
                        // else - not able to read ?... may be disconnect..
                        printf("sock %d no data to tx socket.. disconnecting...\n", tst_var->sock_number);
                        tst_var->connection_status = CLOSE;
                    }
                }
                // else - poll error... may be disconnect..
            }
        }
        //------------------------------------------------------
        else if(tst_var->connection_status == INVALID)
        {
            if(tst_var->socket_handle > 0)
                close(tst_var->socket_handle);
            printf("sock %d INVALID Client or device. Socket closed\n",tst_var->sock_number);
            break;// exit loop and thread.
        }
        else
        {
            if(tst_var->socket_handle > 0)
                close(tst_var->socket_handle);
            if(tst_var->send_communication_init == 2)
                tst_var->connection_status = INVALID;
            else
                tst_var->connection_status = DISCONNECTED;
            tst_var->client_keep_alive_tx_count = 2;
            printf("Sock %d closed\n",tst_var->sock_number);
        }
        //------------------------------------------------------
    } // loop end

deinit:
    // deinit
    free(remote_data_tx_fifo_buff);
    free(remote_data_rx_fifo_buff);
}

void copy_status_flags_n_send(unsigned int tx_size, struct tst *tst)
{
    int temp1;
    unsigned short socket_status_template[32] = {0x1234, 0x1234, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    unsigned long tx_short_size;

    tx_size += (MAX_SOCKET_DATA_TR-1);
    tx_size &= MAX_TX_DATA_SIZE_MASK;
    tx_short_size = tx_size/2;

    socket_status_template[SEQUENCE_NO] = tst->sequence_no++;
    socket_status_template[PACKET_LENGTH] = tx_short_size;
    socket_status_template[PACKET_LENGTH + 1] = tx_short_size >> 16;
    for(temp1 = 0; temp1 < 16; temp1++)
    {
        tst->tx_data_buffer[temp1] = socket_status_template[temp1];
    }
    fifo_write(tst->remote_tx_fifo, (unsigned char*)tst->tx_data_buffer, tx_size);
}

void process_rx(struct tst *tst)
{
    unsigned short so_buff[64];
    unsigned long crnt_dec_img_size;
    if(tst->rx_in_progress == 0)
    {
        if(tst->remote_rx_fifo->filled_length >= (34*2))
        {
            fifo_peek(tst->remote_rx_fifo, (unsigned char*)so_buff, (34*2));
            //  printf("peek rxdata %x %x %x %x\n", so_buff[0], so_buff[1], so_buff[32], so_buff[33]);
            if((so_buff[0] == 0x1234) && (so_buff[1] == 0x1234))// valid command...
            {
                tst->rx_in_progress = 1;
                tst->expected_len = *((unsigned long*)&so_buff[PACKET_LENGTH])*2;// convert to bytes
            }
            else
            {
                // invalid command.. close socket and disconnect..
                printf("sock %d peek rxdata %x %x %x %x\n",tst->sock_number, so_buff[0], so_buff[1], so_buff[32], so_buff[33]);
                tst->connection_status = CLOSE;
                tst->rx_in_progress = 0;
                printf("sock %d camera command error\n",tst->sock_number);
            }
        }
    }
    else
    {
        if(tst->remote_rx_fifo->filled_length >= tst->expected_len) // all data recieved.. process command...
        {
            tst->rx_in_progress = 0;
            tst->remote_keep_alive_count_reload = REMOTE_KEEPALIVETIMEOUT;
            fifo_read(tst->remote_rx_fifo, (unsigned char*)tst->rx_data_buffer, tst->expected_len);
            // printf("sock %d rxed full command size %ld %x %x %x %x\n", tst->sock_number, tst->expected_len, tst->rx_data_buffer[0], tst->rx_data_buffer[32], tst->rx_data_buffer[33], tst->rx_data_buffer[1024]);
            switch(tst->rx_data_buffer[32])
            {
                case 0x8C: // start command..
                    tst->tx_data_buffer[17] =  0x2B; // send view list

                    if (tst->strm == 0) {
                        tst->tx_data_buffer[18] =  0;
                        tst->tx_data_buffer[19] =  0x101;
                    } else {
                        tst->tx_data_buffer[18] =  0x101;
                        tst->tx_data_buffer[19] =  0;
                    }

                    tst->tx_data_buffer[20] =  0;
                    tst->tx_data_buffer[21] =  0;
                    tst->tx_data_buffer[22] =  0;
                    tst->tx_data_buffer[23] =  0;
                    tst->tx_data_buffer[24] =  0;
                    tst->tx_data_buffer[25] =  0;
                    copy_status_flags_n_send(26, tst);
                    tst->client_keep_alive_tx_count = 2;
                    tst->client_keep_alive_tx_status = 0;
                    tst->send_communication_init = 0;
                    break;

                case 0x2B: // view list ok..
                    tst->tx_data_buffer[17] =  0x2C;
                    copy_status_flags_n_send(18, tst);
                    break;

                case 0x2C: // image rx command..
                    crnt_dec_img_size = tst->rx_data_buffer[36+55];
                    crnt_dec_img_size |= ((unsigned long)(tst->rx_data_buffer[36+56]) << 16);
                    if (tst->strm == 0)
                        crnt_dec_img_size -= 192;
                    image_save(crnt_dec_img_size, (unsigned char*)(&tst->rx_data_buffer[100]), (unsigned char*)(&tst->rx_data_buffer[36]), (unsigned char*)(&tst->rx_data_buffer[100])+crnt_dec_img_size);
                    break;

                case 0x5B: // start command..
                    tst->client_keep_alive_tx_status = 0;
                    tst->client_keep_alive_tx_count = 2;
                    break;

            }
        }
    }
}

void process_tx(struct tst *tst)
{
    if(tst->send_communication_init == 1)
    {
        tst->send_communication_init = 2;
        tst->tx_data_buffer[17] = 0x8C;
        tst->tx_data_buffer[18] = 0xAAAA;
        tst->tx_data_buffer[19] = 0xcccc;
        tst->tx_data_buffer[20] = 5; // Linux Pc... for tk1
        tst->tx_data_buffer[21] = 0;
        copy_status_flags_n_send(21, tst);
        printf("sock %d init cmd sent\n", tst->sock_number);
    }
    if(tst->client_keep_alive_tx_status == 0)
    {
        if(tst->client_keep_alive_tx_count == 0)
        {
            tst->client_keep_alive_tx_status = 1;
            tst->tx_data_buffer[17] = 0x5B;
            copy_status_flags_n_send(18, tst);
            // printf("sock %d keep alive cmd sent\n", tst->sock_number);
        }
    }
}

void server_rx(struct tst *tst)
{
    int i, j;
    unsigned char so_buff[MAX_SOCKET_DATA_TR*2];
    if((MAXREMOTE_RX_DATA_FIFO_LENGTH - tst->remote_rx_fifo->filled_length) > (MAX_SOCKET_DATA_TR*2))
    {
        if(ioctl(tst->socket_handle, SIOCINQ, &j) >= 0)
        {
            if(j > MAX_SOCKET_DATA_TR)
                j = MAX_SOCKET_DATA_TR;
            if(j > 0)
            {
                i = read(tst->socket_handle, so_buff, j);
                fifo_write(tst->remote_rx_fifo, so_buff, i);
            }
        }
    }
}

int server_tx(struct tst *tst)
{
    int i, j;
    unsigned char so_buff[MAX_SOCKET_DATA_TR];

    if(tst->remote_tx_fifo->filled_length)
    {
        if(tst->remote_tx_fifo->filled_length > MAX_SOCKET_DATA_TR)
            i = MAX_SOCKET_DATA_TR;
        else
            i = tst->remote_tx_fifo->filled_length;
        j = 0;
        if(ioctl(tst->socket_handle, SIOCOUTQ, &j) >= 0)
        {
            if(j > MAX_SOCKET_DATA_TR*4)
                return 1;
        }
        fifo_read(tst->remote_tx_fifo, so_buff, i);
        j = write(tst->socket_handle, so_buff, i);
        if(j < i)
        {
            if((i-j) < MAX_SOCKET_DATA_TR)
                if((i-j) > 0)
                    fifo_rewind(tst->remote_tx_fifo, (i-j));
        }
    }
    return 0;
}

