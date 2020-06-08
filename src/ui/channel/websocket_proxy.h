#ifndef WEBSOCKET_PROXY_H
#define WEBSOCKET_PROXY_H

#include <QObject>
#include <QWebSocket>
#include <QTimer>

namespace dstore
{
class WebSocketProxy : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketProxy(QObject *parent = nullptr);
    ~WebSocketProxy();

Q_SIGNALS:
    void onMsg(QString msg);

public Q_SLOTS:
    void newWebSocket(QString url);
    void onDisConnected();
    void onConnected();
    void onTextReceived(QString msg);
    void reconnect();

private:
    QWebSocket *m_pWebSocket;
    QTimer *m_pTimer;
    bool connectStatus;
    QString serverUrl;
};
}
#endif // WEBSOCKET_PROXY_H
