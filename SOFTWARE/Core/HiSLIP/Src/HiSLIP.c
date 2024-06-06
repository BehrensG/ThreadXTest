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
NX_TCP_SOCKET server_socket;
NX_TCP_SOCKET sync_socket;
NX_TCP_SOCKET async_socket;

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

err_t hislip_AsyncInitialize(hislip_instr_t* hislip_instr)
{
	err_t err = 0;
	hislip_msg_t msg_rx, msg_tx;

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

	return err;
}


UCHAR  pool_buffer[1024];

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
/*
static void hislip_Callback(struct netconn *conn, enum netconn_evt even, unsigned short len)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if(NETCONN_EVT_RCVPLUS == even)
	{

		static unsigned int test = 1;
		xQueueSendFromISR(hislip_queue, &test, &xHigherPriorityTaskWoken);

	}
}

static struct netconn*  hislip_Bind(unsigned short port)
{

	struct netconn* conn;
	err_t err;

	conn = netconn_new(NETCONN_TCP);
	err = netconn_bind(conn, IP_ADDR_ANY, port);
	err = netconn_listen(conn);

#if LWIP_SO_RCVTIMEO == 1
	netconn_set_recvtimeout(conn, 1000);
#endif

	return conn;
}



static hislip_msg_type_t hislip_Recv(hislip_instr_t* hislip_instr)
{
	err_t err;
	struct netbuf* buf;
	void* data;
	unsigned short len = 0;
	unsigned short offset = 0;
	hislip_msg_t hislip_msg;

	hislip_instr->netbuf.len = 0;

#if LWIP_SO_RCVTIMEO == 1
	netconn_set_recvtimeout(hislip_instr->netconn.newconn, 100);
#endif

	err = netconn_recv(hislip_instr->netconn.newconn, &buf);

	if(0 == err)
	{
		do
		{
			netbuf_data(buf, &data, &len);
			hislip_instr->netbuf.len +=len;

			if(hislip_instr->netbuf.len <= sizeof(hislip_instr->netbuf.data))
			{
				memcpy(hislip_instr->netbuf.data + offset, data, len);

			}
			else
			{

			}
			offset += len;


		}
		while(netbuf_next(buf) >= 0);

		netbuf_delete(buf);

		hislip_msg = hislip_MsgParser(hislip_instr);
	}
	else
	{
		netbuf_delete(buf);

		netconn_close(hislip_instr->netconn.newconn);
		netconn_delete(hislip_instr->netconn.newconn);


		return HISLIP_CONN_ERR;
	}

	return (hislip_msg_type_t)hislip_msg.msg_type;

}



static void hislip_Init(hislip_instr_t* hislip_instr)
{

}


static void hislip_SyncTask(void  *arg)
{
	hislip_instr_t hislip_instr;
	hislip_Init(&hislip_instr);

	hislip_instr.netconn.newconn = (struct netconn*)arg;

	for (;;)
	{
		switch(hislip_Recv(&hislip_instr))
		{
			case Initialize : hislip_Initialize(&hislip_instr);break;
	//		case AsyncInitialize : hislip_AsyncInitialize(&hislip_instr); break;
	//		case AsyncMaximumMessageSize : hislip_AsyncMaximumMessageSize(&hislip_instr);break;
			case DataEnd : hislip_DataEnd(&hislip_instr); break;

			case HISLIP_CONN_ERR :
				{
					if(task_count)
					{
						task_count--;
					}

					vTaskDelete(NULL);

				}; break;

			default : vTaskDelay(pdMS_TO_TICKS(2)); break;
		}
		vTaskDelay(pdMS_TO_TICKS(2));
	}

}

static void hislip_aSyncTask(void  *arg)
{
	hislip_instr_t hislip_instr;
	hislip_Init(&hislip_instr);

	hislip_instr.netconn.newconn = (struct netconn*)arg;

	for (;;)
	{
		switch(hislip_Recv(&hislip_instr))
		{
		//	case Initialize : hislip_Initialize(&hislip_instr);break;
			case AsyncInitialize : hislip_AsyncInitialize(&hislip_instr); break;
			case AsyncMaximumMessageSize : hislip_AsyncMaximumMessageSize(&hislip_instr);break;
			case DataEnd : hislip_DataEnd(&hislip_instr); break;

			case HISLIP_CONN_ERR :
				{
					if(task_count)
					{
						task_count--;
					}

					vTaskDelete(NULL);

				}; break;

			default : vTaskDelay(pdMS_TO_TICKS(2)); break;
		}
		vTaskDelay(pdMS_TO_TICKS(2));
	}

}

static void hislip_ServerTask(void const *argument)
{
	err_t err;
	struct netconn* newconn;
	struct netconn* conn = hislip_Bind(HISLIP_PORT);

	for (;;)
	{
		err = netconn_accept(conn, &newconn);

		if(0 == err)
		{
			if(0 == task_count)
			{
				hislip_sync_handler = xTaskCreateStatic(hislip_SyncTask,"hislip_SyncTask",
						HISLIP_THREAD_STACKSIZE, (void*)newconn, tskIDLE_PRIORITY + 2,
						hislip_sync_buffer, &hislip_sync_control_block);

				task_count++;

			}

			else if(1 == task_count)
			{
				hislip_async_handler = xTaskCreateStatic(hislip_aSyncTask,"hislip_aSyncTask",
						HISLIP_THREAD_STACKSIZE, (void*)newconn, tskIDLE_PRIORITY + 2,
						hislip_async_buffer, &hislip_async_control_block);

				task_count++;
			}
		}

	}
}
*/
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
	}
	else
	{
        nx_tcp_socket_disconnect(hislip_instr->socket, NX_WAIT_FOREVER);
        nx_tcp_server_socket_unaccept(&hislip_instr->socket);
        nx_tcp_server_socket_relisten(&NetXDuoEthIpInstance, HISLIP_PORT, &hislip_instr->socket);
        if(thread_count)
        {
        	thread_count--;
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
					if(thread_count)
					{
						thread_count--;
					}

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
					if(thread_count)
					{
						thread_count--;
					}

					//vTaskDelete(NULL);

				}; break;

			default : tx_thread_sleep(2); break;
		}
    	tx_thread_sleep(2);
    }
}

void server_thread_entry(ULONG thread_input)
{
    UINT status;
    ULONG actual_status;
    NX_PACKET *packet;
    ULONG socket_state;

    status = nx_tcp_socket_create(&NetXDuoEthIpInstance, &server_socket, "TCP Server Socket", NX_IP_NORMAL, NX_FRAGMENT_OKAY,
                                NX_IP_TIME_TO_LIVE, 1024, NX_NULL, NX_NULL);
     if (status != NX_SUCCESS)
     {
       Error_Handler();
     }

    // Bind the socket to the server port
     status = nx_tcp_server_socket_listen(&NetXDuoEthIpInstance, HISLIP_PORT, &server_socket, 5, NX_NULL);

    while(1)
    {

    	nx_tcp_socket_info_get(&server_socket, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &socket_state, NULL, NULL, NULL);

    	  if(socket_state != NX_TCP_ESTABLISHED)
    	  {
    		  // Accept the first client connection
    		  status = nx_tcp_server_socket_accept(&server_socket, NX_WAIT_FOREVER);
    	  }
    		  tx_thread_sleep(10);

		if(1)
		{
			if(0 == thread_count)
			{

				// Create the client 1 thread
				status = tx_thread_create(&sync_thread, "HiSLIP Sync Thread", sync_thread_entry, (ULONG)&server_socket,
								 sync_thread_stack, HISLIP_THREAD_STACKSIZE, 1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);

				thread_count++;
			}
			else if(1 == thread_count)
			{

				// Create the client 1 thread
				tx_thread_create(&async_thread, "HiSLIP aSync Thread", async_thread_entry, (ULONG)&server_socket,
								 async_thread_stack, HISLIP_THREAD_STACKSIZE, 1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);
			}
		}
    }

}

void hislip_CreateTask(void)
{

	tx_thread_create(&server_thread, "HiSLIP Server Thread", server_thread_entry, 0,
	                     server_thread_stack, HISLIP_THREAD_STACKSIZE, 1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);


}

