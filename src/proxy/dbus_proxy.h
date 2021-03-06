#pragma once

#include <dbus/dbus.h>

#include <QDebug>
#include <QFile>
#include <QLocalSocket>
#include <QLocalServer>
#include <QObject>

// #include <QDBusMessage>
// #include <QDBusError>
// #include <QDBusInterface>
// #include <QDBusReply>

#include "filter/dbus_filter.h"
#include "message/dbus_message.h"

class DbusProxy : public QObject
{
    Q_OBJECT

public:
    DbusProxy();
    ~DbusProxy();

    /*
     * 启动监听
     *
     * @param socketPath: socket监听地址
     *
     * @return bool: true:成功 其它:失败
     */
    bool startListenBoxClient(const QString &socketPath);

    /*
     * 连接dbus-daemon
     *
     * @param daemonPath: dbus-daemon地址
     *
     * @return bool: true:成功 其它:失败
     */
    bool startConnectDbusDaemon(const QString &daemonPath);

    /*
     * 保存socket监听地址
     *
     * @param path: socket监听地址
     */
    void saveBoxSocketPath(const QString &path) { socketPath = path; }

    /*
     * 保存dbus-dameon连接地址
     *
     * @param path: dbus-dameon连接地址
     */
    void saveDbusDaemonPath(const QString &path) { daemonPath = path; }

    /*
     * 保存代理对应的appId
     *
     * @param id: appId
     */
    void saveAppId(const QString &id) { appId = id; }

private:
    /*
     * 客户端dbus报文是否需要回复
     *
     * @param header: dbus socket报文
     *
     * @return bool: true:需要回复 其它:不需要
     */
    bool isNeedReply(const Header *header)
    {
        if (header->type == (int)MessageType::METHOD_CALL) {
            return (header->flags & 0x1) == 0;
        }
        return false;
    }

    /*
     * 客户端dbus报文是否为认证报文
     *
     * @param byteArray: dbus socket报文
     *
     * @return bool: true:是 其它:不是
     */
    bool isDbusAuthMsg(const QByteArray &byteArray)
    {
        if (byteArray[0] != 'l' && byteArray[0] != 'B' && !byteArray.contains("BEGIN")) {
            return true;
        }
        return false;
    }

    // QByteArray createFakeReply(const QString &msg)
    // {
    //     // destnation pos = 24
    //     // reply_serial pos = 92
    //     // serial pos = 8
    //     // sender pos 112
    //     QByteArray byteArray(
    //         "l\x03\x01\x01\x42\x00\x00\x00\x10\x00\x00\x00g\x00\x00\x00\x04\x01s\x00(\x00\x00\x00org.freedesktop.DBus.Error.UnknownMethod\x00\x00\x00\x00\x00\x00\x00\x00\x06\x01s\x00\x06\x00\x00\x00:1.585\x00\x00\x05\x01u\x00\x02\x00\x00\x00\b\x01g\x00\x01s\x00\x00\x07\x01s\x00\x06\x00\x00\x00:1.298\x00\x00\x3D\x00\x00\x00org.freedesktop.DBus.Error.AccessDenied, dbus msg hijack test\x00",
    //         186);
    //     // QByteArray byteArray(
    //     // "l\x03\x01\x01\x2C\x00\x00\x00\x08\x00\x00\x00u\x00\x00\x00\x06\x01s\x00\x06\x00\x00\x00:1.332\x00\x00\x04\x01s\x00(\x00\x00\x00org.freedesktop.DBus.Error.AccessDenied\x00\x00\x00\x00\x00\x00\x00\x00\x00\x05\x01u\x00\x02\x00\x00\x00\b\x01g\x00\x01s\x00\x00\x07\x01s\x00\x14\x00\x00\x00org.freedesktop.DBus\x00\x00\x00\x00\x27\x00\x00\x00org.freedesktop.DBus.Error.AccessDenied\x00",
    //     // 180);

    //     // QString string1("ABCDEFGHI");
    //     // QByteArray utfString = string1.toUtf8();
    //     // const char* stringData1 = utfString.constData();
    //     return byteArray;
    // }

    /*
     * 创建指定参数的dbus错误消息
     *
     * @param byteMsg: dbus socket报文
     * @param serial: 报文序列号
     * @param dst: 报文目标地址
     * @param errorName: 报文错误类型
     * @param errorMsg: 报文错误消息
     *
     * @return QByteArray: 报文字节数组
     */
    QByteArray createFakeReplyMsg(const QByteArray &byteMsg, quint32 serial, const QString &dst,
                                  const QString &errorName, const QString &errorMsg);

public:
    DbusFilter filter;
private slots:

    void onNewConnection();
    void onReadyReadClient();
    void onDisconnectedClient();

    // dbus-daemon 服务端回调函数
    void onConnectedServer();
    void onReadyReadServer();
    void onDisconnectedServer();

private:
    // dbus-proxy server, wait for dbus client in box to connect
    QLocalServer *serverProxy;
    // dbus client, be used to connect to the dbus daemon
    QLocalSocket *clientProxy;

    QLocalSocket *boxClient;
    // QLocalSocket *daemonClient;

    bool isConnectDbusDaemon;
    // 客户端地址
    QString boxClientAddr;

    // socket 地址
    QString socketPath;
    // dbus-daemon path
    QString daemonPath;

    QString appId;
};