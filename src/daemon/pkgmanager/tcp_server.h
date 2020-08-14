#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>

class data_proc_inerface {
public:
	virtual void msg_proc(QByteArray &data, QTcpSocket *sock) = 0;
	virtual void disconnect_proc(QString &str) = 0;
    virtual ~data_proc_inerface(){}
};

class tcp_server : public QObject
{
	Q_OBJECT
public:
    explicit tcp_server(uint16_t port, data_proc_inerface *obj, QObject *parent = nullptr);
    ~tcp_server();
	void stop();

signals:

public slots:
	void on_conned();
	void on_disconned();
	void on_readyread();

private:
	QTcpServer *m_server;
	qint64 bytesreceived;
	data_proc_inerface *data_proc;
};
