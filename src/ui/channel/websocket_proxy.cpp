#include "websocket_proxy.h"
#include <QDebug>

namespace dstore
{
WebSocketProxy::WebSocketProxy(QObject *parent) : QObject(parent)
{
    m_pWebSocket = new QWebSocket;
    connectStatus = false;
    m_pTimer = new QTimer(this);

    qDebug()<<"create websocket!";
    connect(m_pWebSocket,&QWebSocket::disconnected,this,&WebSocketProxy::onDisConnected,Qt::AutoConnection);
    connect(m_pWebSocket,&QWebSocket::textMessageReceived,this,&WebSocketProxy::onTextReceived,Qt::AutoConnection);
    connect(m_pWebSocket,&QWebSocket::connected,this,&WebSocketProxy::onConnected,Qt::AutoConnection);
    connect(m_pTimer,&QTimer::timeout,this,&WebSocketProxy::reconnect,Qt::AutoConnection);
}

WebSocketProxy::~WebSocketProxy()
{
    m_pWebSocket->deleteLater();
}

void dstore::WebSocketProxy::newWebSocket(QString url)
{
    qDebug()<<url;
    serverUrl = url;
    connectStatus = false;
    m_pTimer->stop();
    m_pWebSocket->open(QUrl(serverUrl));
}

void WebSocketProxy::onDisConnected()
{
    qDebug()<<"DataReceive websocket is disconnected!!!";
    connectStatus = false;
    m_pTimer->start(3000);/*-<当连接失败时，触发重连计时器，设置计数周期为3秒 */
}

void WebSocketProxy::onConnected()
{
    qDebug()<<"DataReveive websocket is already connect!";
    connectStatus = true;
    m_pTimer->stop();
    qDebug()<<"Address："<<m_pWebSocket->peerAddress();
}

void WebSocketProxy::onTextReceived(QString msg)
{
    qDebug()<<msg;
    Q_EMIT onMsg(msg);
}

void WebSocketProxy::reconnect()
{
    qDebug()<<"try to reconnect!";
    m_pWebSocket->abort();
    m_pWebSocket->open(QUrl(serverUrl));
}
}
