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


void hislip_DataEnd(hislip_instr_t* hislip_instr)
{

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



#define SERVER_PORT 4880
#define MAX_CLIENTS 10
NX_IP my_ip;
NX_PACKET_POOL my_pool;
NX_TCP_SOCKET server_socket;
TX_THREAD client_threads[MAX_CLIENTS];
NX_TCP_SOCKET client_sockets[MAX_CLIENTS];



hislip_instr_t hislip_instr;

void server_thread_entry(ULONG thread_input)
{
    UINT status;
    ULONG client_index = 0;

    // Create and bind the server socket
    nx_tcp_socket_create(&my_ip, &server_socket, "Server Socket", NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, 512, NX_NULL, NX_NULL);
    nx_tcp_server_socket_listen(&my_ip, SERVER_PORT, &server_socket, MAX_CLIENTS, NULL);

    while (1)
    {
        // Accept a client connection
        if (nx_tcp_server_socket_accept(&server_socket, NX_WAIT_FOREVER) == NX_SUCCESS)
        {
            // Create a new client socket and thread for each connection
            nx_tcp_socket_create(&my_ip, &client_sockets[client_index], "Client Socket", NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, 512, NX_NULL, NX_NULL);
            nx_tcp_server_socket_unaccept(&server_socket);
            nx_tcp_server_socket_relisten(&my_ip, SERVER_PORT, &server_socket);

            // Start the client thread
            if(0 == client_index){
            tx_thread_create(&client_threads[client_index], "Client Thread", sync_thread_entry, (ULONG)&client_sockets[client_index],
            		sync_thread_stack, HISLIP_THREAD_STACKSIZE, 1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);
            }else if(0 == client_index)
            {
                tx_thread_create(&client_threads[client_index], "Client Thread", async_thread_entry, (ULONG)&client_sockets[client_index],
                		async_thread_stack, HISLIP_THREAD_STACKSIZE, 1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);
            }

            // Increment client index, wrap around if necessary
            client_index = (client_index + 1) % MAX_CLIENTS;
        }
    }
}

void hislip_CreateTask(void)
{

	tx_thread_create(&server_thread, "HiSLIP Server Thread", server_thread_entry, 0,
	                     server_thread_stack, HISLIP_THREAD_STACKSIZE, 1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);


}

