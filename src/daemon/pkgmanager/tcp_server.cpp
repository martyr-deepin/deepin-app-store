#include "tcp_server.h"
#if defined(Q_OS_LINUX)
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <ctype.h>
#include <net/if.h>
#include <netinet/in.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif

#define UPDATE_TIMEOUT 60
#define TCP_IDLE_TIME 60
#define TCP_INTVL_TIME 30
#define TCP_CONNECT_CNT 3


static int set_sockopt(int sockfd)
{
	int tcp_nodely = 1;
	int keep_alive = 1; // 开启keepalive属性
	int keep_idle = TCP_IDLE_TIME; //TCP_IDLE_TIME秒内没有任何数据往来,则进行探测
	int keep_interval = TCP_INTVL_TIME; //TCP_INTVL_TIME 秒
	int keep_count = TCP_CONNECT_CNT; //TCP_CONNECT_CNT次的不再发
	struct timeval timeout;

	timeout.tv_sec = UPDATE_TIMEOUT;
	timeout.tv_usec = 0;
	int ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO,
						 &timeout, sizeof(timeout));
	if (ret < 0) {
		return -1;
	}

	timeout.tv_sec = UPDATE_TIMEOUT;
	timeout.tv_usec = 0;
	ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
					 &timeout, sizeof(timeout));
	if (ret < 0) {
		return -1;
	}

	if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,
				   &tcp_nodely, sizeof(tcp_nodely)) < 0) {
		return -1;
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE,
				   &keep_alive, sizeof(keep_alive)) < 0) {
		return -1;
	}

	if (setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE,
				   &keep_idle, sizeof(keep_idle)) < 0) {
		return -1;
	}

	if (setsockopt(sockfd, SOL_TCP, TCP_KEEPINTVL,
				   &keep_interval, sizeof(keep_interval)) < 0) {
		return -1;
	}

	if (setsockopt(sockfd, SOL_TCP, TCP_KEEPCNT,
				   &keep_count, sizeof(keep_count)) < 0) {
		return -1;
	}

	return 0;
}


tcp_server::tcp_server(uint16_t port, data_proc_inerface *obj,
					   QObject *parent) :
	QObject(parent),
	data_proc(obj)
{
	bytesreceived = 0;
	m_server = new QTcpServer(this);
	m_server->listen(QHostAddress::Any, port);
	connect(m_server, &QTcpServer::newConnection,
			this, &tcp_server::on_conned);
}

tcp_server::~tcp_server()
{
	stop();
}

void tcp_server::stop()
{
	if (m_server) {
		disconnect(m_server);
		delete m_server;
		m_server = nullptr;
	}
}

void tcp_server::on_conned()
{
	QTcpSocket *m_socket = m_server->nextPendingConnection();
#if defined(Q_OS_LINUX)
	set_sockopt(m_socket->socketDescriptor());
#endif
	connect(m_socket, SIGNAL(disconnected()), this, SLOT(on_disconned()));
    connect(m_socket, SIGNAL(disconnected()), m_socket, SLOT(deleteLater()));
	connect(m_socket, SIGNAL(readyRead()), this, SLOT(on_readyread()));
}

void tcp_server::on_disconned()
{
	QTcpSocket *sock = qobject_cast<QTcpSocket *>(QObject::sender());
	uint32_t v4 = sock->peerAddress().toIPv4Address();
    in_addr v4i;
	v4i.s_addr = ntohl(v4);
	QString str(inet_ntoa(v4i));
	//qPrintable(str);
	sock->close();
	data_proc->disconnect_proc(str);
}

void tcp_server::on_readyread()
{
	QTcpSocket *sock = qobject_cast<QTcpSocket *>(QObject::sender());
	QByteArray data = sock->readAll();

	data_proc->msg_proc(data, sock);
}
