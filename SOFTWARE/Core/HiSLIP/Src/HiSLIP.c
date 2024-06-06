/*
 * HiSLIP.c
 *
 *  Created on: Jun 3, 2024
 *      Author: grzegorz
 */


 #include <string.h>
#include <stdbool.h>
// #include "stddef.h"
#include "app_azure_rtos.h"
#include "app_netxduo.h"

#include "main.h"

#include "HiSLIP.h"
#include "SCPI_Def.h"

#define HISLIP_THREAD_STACKSIZE		2048
#define HISLIP_PORT					4880

#define	NETCONN_ACCEPT_ON			(unsigned char)1
#define	NETCONN_ACCEPT_OFF			(unsigned char)0


typedef struct
{
	char data[HISLIP_MAX_DATA_SIZE + sizeof(hislip_msg_t)];
	unsigned short len;
}hislip_netbuf_t;

typedef struct {
	hislip_msg_t msg;
	hislip_netbuf_t netbuf;
	NX_PACKET* packet;
	NX_TCP_SOCKET* socket;
	unsigned short session_id;
}hislip_instr_t;

#define err_t	unsigned int

static	unsigned char thread_count = 0;


TX_THREAD server_thread;
TX_THREAD sync_thread;
TX_THREAD async_thread;
UCHAR server_thread_stack[HISLIP_THREAD_STACKSIZE];
UCHAR sync_thread_stack[HISLIP_THREAD_STACKSIZE];
UCHAR async_thread_stack[HISLIP_THREAD_STACKSIZE];
extern NX_IP          NetXDuoEthIpInstance;

#define APP_TCP_INSTANT_ECHO	1

static hislip_msg_t hislip_MsgParser(hislip_instr_t* hislip_instr);

// ----------------------------------------------------------------------------

static void hislip_htonl(hislip_msg_t* hislip_msg)
{
	hislip_msg->msg_param = htonl(hislip_msg->msg_param);
	hislip_msg->prologue = htons(hislip_msg->prologue);
	hislip_msg->payload_len.hi = htonl(hislip_msg->payload_len.hi);
	hislip_msg->payload_len.lo = htonl(hislip_msg->payload_len.lo);
}

size_t hislip_SumSize(size_t* sizes, size_t len)
{
	size_t sum = 0;

	for(unsigned char i = 0; i < len; i++)
	{
		sum += sizes[i];
	}

	return sum;
}

void hislip_CopyMemory(char* destination, void** sources, size_t* sizes, unsigned int num_sources)
{
    size_t offset = 0;
    for (unsigned int i = 0; i < num_sources; i++)
    {
        memcpy(destination + offset, sources[i], sizes[i]);
        offset += sizes[i];
    }
}

// ----------------------------------------------------------------------------


err_t hislip_DataEnd(hislip_instr_t* hislip_instr)
{
	err_t err = 0;

	hislip_msg_t msg_rx;

	char* buf;
	char* end;

	const char* ends[3] = {LINE_ENDING_CR, LINE_ENDING_LF, LINE_ENDING_CRLF};

	msg_rx = hislip_MsgParser(hislip_instr);

	buf = &hislip_instr->netbuf.data[sizeof(hislip_msg_t)];

	for(unsigned char i = 0; i < 3; i++)
	{
		end = strstr(buf, ends[i]);
		if(NULL != end)
		{
			break;
		}

	}

	if(NULL != end)
	{
		memcpy(end, SCPI_LINE_ENDING, strlen(SCPI_LINE_ENDING));
	}
	else
	{
		memcpy(buf + msg_rx.payload_len.lo, SCPI_LINE_ENDING, strlen(SCPI_LINE_ENDING));
	}

	//SCPI_Input(&scpi_context, buf, msg_rx.payload_len.lo + strlen(SCPI_LINE_ENDING));


	return err;
}

/*
err_t hislip_DataEnd(hislip_instr_t* hislip_instr)
{
	err_t err = 0;

	hislip_msg_t msg_rx, msg_tx;
	payload_len_t max_msg_size;

	char data[] = "ABCad,123123,432,123213\n";

	void* sources[] = {&msg_tx, &data};
	size_t sizes[] = {sizeof(hislip_msg_t), strlen(data)};

	msg_rx = hislip_MsgParser(hislip_instr);

	size_t sum = hislip_SumSize(sizes, 2);

	char payload[sum];

	msg_tx.prologue = HISLIP_PROLOGUE;
	msg_tx.msg_type = HISLIP_DATAEND;
	msg_tx.control_code = 0x00;
	msg_tx.msg_param = msg_rx.msg_param;
	msg_tx.payload_len.hi = 0;
	msg_tx.payload_len.lo = strlen(data);

	hislip_htonl(&msg_tx);

	hislip_CopyMemory(payload, sources, sizes, 2);

	vTaskDelay(pdMS_TO_TICKS(1));
	err = netconn_write(hislip_instr->netconn.newconn, payload, sum, NETCONN_NOFLAG);

	return err;
}
*/


err_t hislip_AsyncMaximumMessageSize(hislip_instr_t* hislip_instr)
{
	err_t err = 0;
	hislip_msg_t msg_tx;
	payload_len_t max_msg_size;


	void* sources[] = {&msg_tx, &max_msg_size};
	size_t sizes[] = {sizeof(hislip_msg_t), sizeof(payload_len_t)};

	size_t sum = hislip_SumSize(sizes, 2);

	char payload[sum];

	msg_tx.prologue = HISLIP_PROLOGUE;
	msg_tx.msg_type = HISLIP_ASYNCMAXIMUMMESSAGESIZERESPONSE;
	msg_tx.control_code = 0x00;
	msg_tx.msg_param = 0x00000000;
	msg_tx.payload_len.hi = 0;
	msg_tx.payload_len.lo = 8;

	max_msg_size.hi = 0;
	max_msg_size.lo = 1024;

	hislip_htonl(&msg_tx);

	hislip_CopyMemory(payload, sources, sizes, 2);

	//vTaskDelay(pdMS_TO_TICKS(1));
	//err = netconn_write(hislip_instr->netconn.newconn, payload, sum, NETCONN_NOFLAG);

	return err;
}

UCHAR  pool_buffer[1024];

err_t hislip_AsyncInitialize(hislip_instr_t* hislip_instr)
{
	err_t err = 0;
	hislip_msg_t msg_rx, msg_tx;

	NX_PACKET_POOL* pool;
	NX_PACKET* packet;
	unsigned int status;

	msg_rx = hislip_MsgParser(hislip_instr);

	hislip_instr->session_id = msg_rx.msg_param;

	msg_tx.prologue = HISLIP_PROLOGUE;
	msg_tx.msg_type = HISLIP_ASYNCINITIALIZERESPONSE;
	msg_tx.control_code = 0x00;
	msg_tx.msg_param = HISLIP_VENDOR_ID;
	msg_tx.payload_len.hi = 0;
	msg_tx.payload_len.lo = 0;

	hislip_htonl(&msg_tx);

	//vTaskDelay(pdMS_TO_TICKS(1));
	//err = netconn_write(hislip_instr->netconn.newconn, &msg_tx, sizeof(hislip_msg_t), NETCONN_NOFLAG);

	tx_thread_sleep(1);
	status = nx_packet_pool_create(pool, "Test pool",512, pool_buffer, 512  + sizeof(NX_PACKET));
	status = nx_packet_allocate(pool, &packet, NX_TCP_PACKET, NX_WAIT_FOREVER);

	status = nx_packet_data_append(packet,&msg_tx,sizeof(hislip_msg_t),pool,NX_WAIT_FOREVER);


	        /* immediately resend the same packet */
	status = nx_tcp_socket_send(hislip_instr->socket, packet, NX_WAIT_FOREVER);

	nx_packet_release(packet);
	nx_packet_pool_delete(pool);

	return err;
}




err_t hislip_Initialize(hislip_instr_t* hislip_instr)
{
	err_t err = 0;
	char name_ref[] = "hislip0";
	char name_rx[12];
	char* data;
	hislip_msg_t msg_rx, msg_tx;

	NX_PACKET_POOL* pool;
	NX_PACKET* packet;

	memset(name_rx, 0, 12);

	msg_rx = hislip_MsgParser(hislip_instr);

	data = &hislip_instr->netbuf.data;

	memcpy(name_rx, data + sizeof(hislip_msg_t), msg_rx.payload_len.lo);

	if(!strcmp(name_ref, name_rx))
	{
		msg_tx.prologue = HISLIP_PROLOGUE;
		msg_tx.msg_type = HISLIP_INITIALIZERESPONSE;
		msg_tx.control_code = 0x00;
		msg_tx.msg_param = 0x01000001;
		msg_tx.payload_len.hi = 0;
		msg_tx.payload_len.lo = 0;

		hislip_htonl(&msg_tx);

		unsigned int status;
		//vTaskDelay(pdMS_TO_TICKS(1));
		tx_thread_sleep(1);
		status = nx_packet_pool_create(pool, "Test pool",512, pool_buffer,512  + sizeof(NX_PACKET));
		status = nx_packet_allocate(pool, &packet, NX_TCP_PACKET, NX_WAIT_FOREVER);

		status = nx_packet_data_append(packet,&msg_tx,sizeof(hislip_msg_t),pool,NX_WAIT_FOREVER);


		        /* immediately resend the same packet */
		status = nx_tcp_socket_send(hislip_instr->socket, packet, NX_WAIT_FOREVER);

		nx_packet_release(packet);
		nx_packet_pool_delete(pool);
		//err = netconn_write(hislip_instr->netconn.newconn, &msg_tx, sizeof(hislip_msg_t), NETCONN_NOFLAG);
	}

	return err;
}

// ----------------------------------------------------------------------------


static hislip_msg_t hislip_MsgParser(hislip_instr_t* hislip_instr)
{
	hislip_msg_t hislip_msg;


	size_t hislip_msg_size = sizeof(hislip_msg_t);

	memcpy(&hislip_msg, hislip_instr->netbuf.data, hislip_msg_size);


	hislip_htonl(&hislip_msg);

	return hislip_msg;
}

static hislip_msg_type_t hislip_Recv(hislip_instr_t* hislip_instr)
{

	unsigned int status;

	unsigned short len = 0;
	unsigned short offset = 0;
	hislip_msg_t hislip_msg;

	hislip_instr->netbuf.len = 0;


	status = nx_tcp_socket_receive(hislip_instr->socket, &hislip_instr->packet, NX_WAIT_FOREVER);

	if(NX_SUCCESS == status)
	{
		nx_packet_data_retrieve(hislip_instr->packet, hislip_instr->netbuf.data, &hislip_instr->netbuf.len);

		hislip_msg = hislip_MsgParser(hislip_instr);

     //   nx_tcp_socket_disconnect(hislip_instr->socket, NX_WAIT_FOREVER);
     //  nx_tcp_server_socket_unaccept(&hislip_instr->socket);
     //   nx_tcp_server_socket_relisten(&NetXDuoEthIpInstance, HISLIP_PORT, &hislip_instr->socket);
	}
	else
	{

        if(thread_count)
        {
        	//thread_count--;
        }

		return HISLIP_CONN_ERR;
	}

	return (hislip_msg_type_t)hislip_msg.msg_type;

}


void async_thread_entry(ULONG thread_input)
{
	hislip_instr_t hislip_instr;
	hislip_instr.socket = (NX_TCP_SOCKET *)thread_input;

	HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);

    while (1)
    {
    	switch(hislip_Recv(&hislip_instr))
		{
		//	case Initialize : hislip_Initialize(&hislip_instr);break;
			case AsyncInitialize : hislip_AsyncInitialize(&hislip_instr); break;
			case AsyncMaximumMessageSize : hislip_AsyncMaximumMessageSize(&hislip_instr);break;
			case DataEnd : hislip_DataEnd(&hislip_instr); break;

			case HISLIP_CONN_ERR :
				{
			        nx_tcp_socket_disconnect(hislip_instr.socket, NX_WAIT_FOREVER);
			        nx_tcp_server_socket_unaccept(&hislip_instr.socket);
			        nx_tcp_server_socket_relisten(&NetXDuoEthIpInstance, HISLIP_PORT, &hislip_instr.socket);
					HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
					//vTaskDelete(NULL);

				}; break;

			default : tx_thread_sleep(2); break;
		}
    	tx_thread_sleep(2);
    }
}


void sync_thread_entry(ULONG thread_input)
{
	hislip_instr_t hislip_instr;
	hislip_instr.socket = (NX_TCP_SOCKET *)thread_input;

	HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);

    while (1)
    {
    	switch(hislip_Recv(&hislip_instr))
		{
			case Initialize : hislip_Initialize(&hislip_instr);break;
			case AsyncInitialize : hislip_AsyncInitialize(&hislip_instr); break;
			case AsyncMaximumMessageSize : hislip_AsyncMaximumMessageSize(&hislip_instr);break;
			case DataEnd : hislip_DataEnd(&hislip_instr); break;

			case HISLIP_CONN_ERR :
				{
			        nx_tcp_socket_disconnect(hislip_instr.socket, NX_WAIT_FOREVER);
			        nx_tcp_server_socket_unaccept(&hislip_instr.socket);
			        nx_tcp_server_socket_relisten(&NetXDuoEthIpInstance, HISLIP_PORT, &hislip_instr.socket);

					//vTaskDelete(NULL);
					HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);

				}; break;

			default : tx_thread_sleep(2); break;
		}
    	tx_thread_sleep(2);
    }
}
#define APP_TCP_SOCKET_NUM 2

static NX_TCP_SOCKET g_tcp_sck[APP_TCP_SOCKET_NUM];

static ULONG g_not_listening = TX_FALSE;

static void g_tcp_sck_listen_cb(NX_TCP_SOCKET *, UINT);
static void g_tcp_sck_receive_cb(NX_TCP_SOCKET *);
static void g_tcp_sck_disconn_cb(NX_TCP_SOCKET *);

/* TCP Thread entry function */
void tcp_thread_entry(void)
{
    UINT status;

    /* Create all sockets */
    for (INT i = 0; i < APP_TCP_SOCKET_NUM; i++)
    {
        status = nx_tcp_socket_create(&NetXDuoEthIpInstance, g_tcp_sck + i, "TCP Socket",
                                      NX_IP_NORMAL, NX_FRAGMENT_OKAY,
                                      NX_IP_TIME_TO_LIVE, 512, NX_NULL,
                                      g_tcp_sck_disconn_cb);
        if (NX_SUCCESS != status)
        {
            __BKPT(0);
        }
    }

#if APP_TCP_RTT_LOG
    SEGGER_RTT_printf(0, "Created %u TCP socket%s on port %lu\n",
                      APP_TCP_SOCKET_NUM, APP_TCP_SOCKET_NUM > 1 ? "s" : "",
                      APP_TCP_PORT);
#endif

    /* Start listening on the first socket */
    status = nx_tcp_server_socket_listen(&NetXDuoEthIpInstance, HISLIP_PORT, g_tcp_sck, 0,
                                         g_tcp_sck_listen_cb);
    if (NX_SUCCESS != status)
    {
        //__BKPT(0);
    	tx_thread_sleep(2);
    }

    while (1)
    {
#if APP_TCP_INSTANT_ECHO
        /* Everything else is handled from TCP callbacks dispatched from
         * IP instance thread */
        tx_thread_suspend(tx_thread_identify());
#else
        /* Receive pointers to TCP socket and network packet from
         * TCP callback */
        ULONG msg[2];
        status = tx_queue_receive(&g_tcp_q, msg, TX_WAIT_FOREVER);

        if (TX_SUCCESS == status)
        {
            NX_TCP_SOCKET * p_sck    = (NX_TCP_SOCKET *) msg[0];
            NX_PACKET     * p_packet = (NX_PACKET *)     msg[1];

            status = nx_tcp_socket_send(p_sck, p_packet, NX_NO_WAIT);
            if (NX_SUCCESS != status)
            {
                nx_packet_release(p_packet);
            }
        }

        else
        {
            /* Unrecoverable error */
            tx_thread_suspend(tx_thread_identify());
        }
#endif
    }
}


static void g_tcp_sck_listen_cb(NX_TCP_SOCKET * p_sck, UINT port)
{
    /* Incoming connection, accept and queueing new requests */
    nx_tcp_server_socket_accept(p_sck, NX_NO_WAIT);
 //   nx_tcp_server_socket_unlisten(&NetXDuoEthIpInstance, port);
    nx_tcp_socket_receive_notify(p_sck, g_tcp_sck_receive_cb);

#if APP_TCP_RTT_LOG
    ULONG ip, client_port;
    nx_tcp_socket_peer_info_get(p_sck, &ip, &client_port);
    SEGGER_RTT_printf(0, "[TCP %x] Incoming connection from %lu.%lu.%lu.%lu:%lu\n",
                      p_sck, (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
                      (ip >> 8) & 0xFF, (ip) & 0xFF, client_port);
#endif

    /* Attempt to find another idle socket to start listening on */
    ULONG state = 0;

    for (INT i = 0; i < APP_TCP_SOCKET_NUM; i++)
    {
        /* Get socket state value */
        nx_tcp_socket_info_get(g_tcp_sck + i,
                               0, 0, 0, 0, 0, 0, 0, &state, 0, 0, 0);

        /* Start lisnening if socket is idle */
        if (NX_TCP_CLOSED == state)
        {
            nx_tcp_server_socket_listen(&NetXDuoEthIpInstance, port, g_tcp_sck + i, 0,
                                        g_tcp_sck_listen_cb);

            break;
        }
    }

    /* Ran out of sockets, set appropriate flag to let next socket to
     * disconnect know that it should start listening right away. */
    if (NX_TCP_CLOSED != state)
    {
        g_not_listening = TX_TRUE;
    }
}
hislip_instr_t hislip_instr;

static void g_tcp_sck_receive_cb(NX_TCP_SOCKET * p_sck)
{
    NX_PACKET * p_packet;

    /* This callback is invoked when data is already received. Retrieving
     * packet with no suspension. */
  //  nx_tcp_socket_receive(p_sck, &p_packet, NX_NO_WAIT);

#if APP_TCP_RTT_LOG
    ULONG ip, client_port;
    nx_tcp_socket_peer_info_get(p_sck, &ip, &client_port);
    SEGGER_RTT_printf(0, "[TCP %x] Incoming packet (%lu bytes) from %lu.%lu.%lu.%lu:%lu\n",
                      p_sck, p_packet->nx_packet_length, (ip >> 24) & 0xFF,
                      (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, (ip) & 0xFF, client_port);
#endif
   // hislip_instr.packet = p_packet;
    hislip_instr.socket = p_sck;
#if APP_TCP_INSTANT_ECHO
    /* Send packet back on the same TCP socket */
    switch(hislip_Recv(&hislip_instr))
    		{
    			case Initialize : hislip_Initialize(&hislip_instr);break;
    			case AsyncInitialize : hislip_AsyncInitialize(&hislip_instr); break;
    			case AsyncMaximumMessageSize : hislip_AsyncMaximumMessageSize(&hislip_instr);break;
    			case DataEnd : hislip_DataEnd(&hislip_instr); break;

    			case HISLIP_CONN_ERR :
    				{
    			        nx_tcp_socket_disconnect(hislip_instr.socket, NX_WAIT_FOREVER);
    			        nx_tcp_server_socket_unaccept(&hislip_instr.socket);
    			        nx_tcp_server_socket_relisten(&NetXDuoEthIpInstance, HISLIP_PORT, &hislip_instr.socket);

    					//vTaskDelete(NULL);
    					HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);

    				}; break;

    			default : tx_thread_sleep(2); break;
    		}
#else
    ULONG msg[2] =
    {
        (ULONG) p_sck, (ULONG) p_packet
    };

    /* Sent TCP socket and packet pointers to user thread */
    if (TX_SUCCESS != tx_queue_send(&g_tcp_q, msg, TX_NO_WAIT))
    {
        nx_packet_release(p_packet);
    }
#endif
}


static void g_tcp_sck_disconn_cb(NX_TCP_SOCKET * p_sck)
{
#if APP_TCP_RTT_LOG
    SEGGER_RTT_printf(0, "[TCP %x] Client disconnected\n", p_sck);
#endif

    nx_tcp_server_socket_unaccept(p_sck);

    /* If all sockets are busy, start listening again */
    if (TX_TRUE == g_not_listening)
    {
        nx_tcp_server_socket_listen(&NetXDuoEthIpInstance, HISLIP_PORT, p_sck, 0,
                                    g_tcp_sck_listen_cb);

        g_not_listening = TX_FALSE;
    }
}

void hislip_CreateTask(void)
{

	tx_thread_create(&server_thread, "HiSLIP Server Thread", tcp_thread_entry, 0,
	                     server_thread_stack, HISLIP_THREAD_STACKSIZE, 1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);


}

