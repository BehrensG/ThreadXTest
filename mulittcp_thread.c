#include "tcp_thread.h"

#include "rtt/SEGGER_RTT.h"

#define APP_TCP_SOCKET_NUM      (3)
#define APP_TCP_PORT            (5000)
#define APP_TCP_INSTANT_ECHO    (1)
#define APP_TCP_RTT_LOG         (1)

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
        status = nx_tcp_socket_create(&g_ip, g_tcp_sck + i, "TCP Socket",
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
    status = nx_tcp_server_socket_listen(&g_ip, APP_TCP_PORT, g_tcp_sck, 0,
                                         g_tcp_sck_listen_cb);
    if (NX_SUCCESS != status)
    {
        __BKPT(0);
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
    nx_tcp_server_socket_unlisten(&g_ip, port);
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
            nx_tcp_server_socket_listen(&g_ip, port, g_tcp_sck + i, 0,
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


static void g_tcp_sck_receive_cb(NX_TCP_SOCKET * p_sck)
{
    NX_PACKET * p_packet;

    /* This callback is invoked when data is already received. Retrieving
     * packet with no suspension. */
    nx_tcp_socket_receive(p_sck, &p_packet, NX_NO_WAIT);

#if APP_TCP_RTT_LOG
    ULONG ip, client_port;
    nx_tcp_socket_peer_info_get(p_sck, &ip, &client_port);
    SEGGER_RTT_printf(0, "[TCP %x] Incoming packet (%lu bytes) from %lu.%lu.%lu.%lu:%lu\n",
                      p_sck, p_packet->nx_packet_length, (ip >> 24) & 0xFF,
                      (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, (ip) & 0xFF, client_port);
#endif

#if APP_TCP_INSTANT_ECHO
    /* Send packet back on the same TCP socket */
    nx_tcp_socket_send(p_sck, p_packet, NX_NO_WAIT);
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
        nx_tcp_server_socket_listen(&g_ip, APP_TCP_PORT, p_sck, 0,
                                    g_tcp_sck_listen_cb);

        g_not_listening = TX_FALSE;
    }
}