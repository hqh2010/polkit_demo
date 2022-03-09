#include "dbus_proxy.h"

#include <unistd.h>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

DbusProxy::DbusProxy()
    : isConnectDbusDaemon(false)
{
    // dbus-proxy server, wait for dbus client in box to connect
    serverProxy = new QLocalServer();
    // dbus client, be used to connect to the dbus daemon
    clientProxy = new QLocalSocket();
    connect(serverProxy, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
}

DbusProxy::~DbusProxy()
{
    if (serverProxy) {
        serverProxy->close();
        delete serverProxy;
    }
    if (clientProxy) {
        clientProxy->close();
        delete clientProxy;
    }
}

/*
 * 启动监听
 *
 * @param socketPath: socket监听地址
 *
 * @return bool: true:成功 其它:失败
 */
bool DbusProxy::startListenBoxClient(const QString &socketPath)
{
    if (socketPath.isEmpty()) {
        qCritical() << "socketPath is empty";
        return false;
    }
    QLocalServer::removeServer(socketPath);
    serverProxy->setSocketOptions(QLocalServer::UserAccessOption);
    bool ret = serverProxy->listen(socketPath);
    if (!ret) {
        qCritical() << "listen box dbus client error";
        return false;
    }
    qInfo() << "startListenBoxClient ret:" << ret;
    return ret;
}

/*
 * 连接dbus-daemon
 *
 * @param daemonPath: dbus-daemon地址
 *
 * @return bool: true:成功 其它:失败
 */
bool DbusProxy::startConnectDbusDaemon(const QString &daemonPath)
{
    if (daemonPath.isEmpty()) {
        qCritical() << "daemonPath is empty";
        return false;
    }
    // bind clientProxy to dbus daemon
    connect(clientProxy, SIGNAL(connected()), this, SLOT(onConnectedServer()));
    connect(clientProxy, SIGNAL(disconnected()), this, SLOT(onDisconnectedServer()));
    connect(clientProxy, SIGNAL(readyRead()), this, SLOT(onReadyReadServer()));
    // connect (clientProxy, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWritten_callback(qint64)));
    qInfo() << "start connect dbus-daemon...";
    clientProxy->connectToServer(daemonPath);
    // 等待5s
    if (!clientProxy->waitForConnected(5000)) {
        qCritical() << "connect dbus-daemon error, msg:" << clientProxy->errorString();
        return false;
    }
    return true;
}

void DbusProxy::onNewConnection()
{
    qInfo() << "onNewConnection called, server: " << serverProxy->serverName();
    QLocalSocket *client = serverProxy->nextPendingConnection();
    connect(client, SIGNAL(readyRead()), this, SLOT(onReadyReadClient()));
    connect(client, SIGNAL(disconnected()), this, SLOT(onDisconnectedClient()));
}

// QString getServiceAddr(const QString &serviceName)
// {
//     // com.deepin.linglong.AppManager
//     QDBusInterface interface("org.freedesktop.DBus", "/", "org.freedesktop.DBus", QDBusConnection::sessionBus());
//     QDBusPendingReply<QString> reply = interface.call("GetNameOwner", serviceName);
//     reply.waitForFinished();
//     QString ret = "";
//     if (reply.isValid()) {
//         ret = reply.value();
//     }
//     return ret;
// }

int requestPermission(const QString &appId)
{
    QDBusInterface interface("org.desktopspec.permission", "/org/desktopspec/permission", "org.desktopspec.permission",
                             QDBusConnection::sessionBus());
    // 20s dbus 客户端默认25s必须回
    interface.setTimeout(1000 * 60 * 20);
    QDBusPendingReply<int> reply = interface.call("request", appId, "org.desktopspec.permission.Account");
    reply.waitForFinished();
    int ret = -1;
    if (reply.isValid()) {
        ret = reply.value();
    }
    qInfo() << "requestPermission ret:" << ret;
    return ret;
}

void DbusProxy::onReadyReadClient()
{
    // box client socket address
    boxClient = static_cast<QLocalSocket *>(sender());
    if (boxClient) {
        QByteArray data = boxClient->readAll();
        qInfo() << "Read Data From Client size:" << data.size();
        qInfo() << "Read Data From Client:" << data;
        // 去掉数据末尾的\r\n和第一个无效字节
        // data.replace("\r\n", "");
        // QByteArray dataSend = data.mid(1);
        // qInfo() << "Read Data From Client valid data:" << dataSend;
        // QString dataString = data;
        // int pos = dataString.indexOf("BEGIN");
        QByteArray helloData;
        bool isHelloMsg = data.contains("BEGIN");
        if (isHelloMsg) {
            // auth begin msg is not normal header
            qWarning() << "get client hello msg";
            helloData = data.mid(7);
        }

        Header header;
        bool ret = false;
        if (isHelloMsg) {
            ret = parseHeader(helloData, &header);
        } else {
            ret = parseHeader(data, &header);
        }
        if (!ret) {
            qWarning() << "parseHeader is not a normal msg";
        }
        if (!isConnectDbusDaemon) {
            ret = startConnectDbusDaemon(daemonPath);
            qInfo() << "start reconnect dbus-daemon ret:" << ret;
        }

        // 判断是否满足过滤规则
        bool isMatch = filter.isMessageMatch(header.destination, header.path, header.interface);
        qInfo() << "msg destination:" << header.destination << ", header.path:" << header.path
                << ", header.interface:" << header.interface << ", dbus msg match filter ret:" << isMatch;
        // 握手信息不拦截
        if (!isDbusAuthMsg(data) && !isMatch) {
            // 未配置权限申请用户授权
            int result = requestPermission(appId);
            if (result != 2) {
                if (isNeedReply(&header)) {
                    QByteArray reply = createFakeReplyMsg(
                        data, header.serial + 1, boxClientAddr, "org.freedesktop.DBus.Error.AccessDenied",
                        "org.freedesktop.DBus.Error.AccessDenied, dbus msg hijack test");

                    // 伪造 错误消息格式给客户端
                    // 将消息发送方 header中的serial 填充到 reply_serial
                    // 填写消息类型 flags(是否需要回复) 消息body 需要修改消息body长度
                    // 生成一个惟一的序列号
                    boxClient->write(reply);
                    boxClient->waitForBytesWritten(3000);
                    boxClient->flush();
                    qInfo() << "reply size:" << reply.size();
                    qInfo() << reply;
                }
                return;
            }
        }
        if (isConnectDbusDaemon) {
            clientProxy->write(data);
            qInfo() << "send data to dbus-daemon done";
        }
    }
}

void DbusProxy::onDisconnectedClient()
{
    QLocalSocket *sender = static_cast<QLocalSocket *>(QObject::sender());
    if (sender) {
        sender->disconnectFromServer();
    }
    qInfo() << "onDisconnectedClient called sender:" << sender;

    if (clientProxy) {
        clientProxy->disconnectFromServer();
    }
}

// dbus-daemon 服务端回调函数
void DbusProxy::onConnectedServer()
{
    qInfo() << "connected to dbus-daemon:" << clientProxy->fullServerName() << " success";
    isConnectDbusDaemon = true;
}

void DbusProxy::onReadyReadServer()
{
    // daemonClient = static_cast<QLocalSocket *>(QObject::sender());
    QByteArray receiveDta = clientProxy->readAll();
    qInfo() << "receive from dbus-daemon:";
    qInfo() << receiveDta;
    qInfo() << "parse msg header from dbus-daemon";
    Header header;
    bool ret = parseHeader(receiveDta, &header);
    if (!ret) {
        qCritical() << "dbus-daemon msg parseHeader err";
    }
    // is a right way to judge?
    bool isHelloReply = receiveDta.contains("NameAcquired");
    if (isHelloReply) {
        boxClientAddr = header.destination;
        qInfo() << "boxClientAddr:" << boxClientAddr;
    }

    // 将消息转发给客户端
    if (boxClient) {
        boxClient->write(receiveDta);
        qInfo() << "send data to box dbus client done,data size:" << receiveDta.size();
    }
}

// 与dbus-daemon 断开连接
void DbusProxy::onDisconnectedServer()
{
    QLocalSocket *sender = static_cast<QLocalSocket *>(QObject::sender());
    if (sender) {
        sender->disconnectFromServer();
    }
    isConnectDbusDaemon = false;
    qInfo() << "onDisconnectedServer called sender:" << sender;
}

QByteArray DbusProxy::createFakeReplyMsg(const QByteArray &byteMsg, quint32 serial, const QString &dst,
                                         const QString &errorName, const QString &errorMsg)
{
    // 构造一个空的错误消息
    // DBusMessage *dbusMsg = dbus_message_new(DBUS_MESSAGE_TYPE_ERROR);
    // if (!dbusMsg) {
    //     qCritical() << "createFakeReplyMsg create msg failed";
    // }

    // quint32 replySerial = 2;
    // auto ret = dbus_message_set_reply_serial(dbusMsg, replySerial);
    // if (!ret) {
    //     qCritical() << "createFakeReplyMsg set reply_serial failed";
    // }

    // quint32 serial = 3;
    // dbus_message_set_serial(dbusMsg, serial);

    // const char *sender = ":1.312";
    // ret = dbus_message_set_sender(dbusMsg, sender);
    // if (!ret) {
    //     qCritical() << "createFakeReplyMsg set sender failed";
    // }

    // const char *destination = ":1.315";
    // ret = dbus_message_set_destination(dbusMsg, destination);
    // if (!ret) {
    //     qCritical() << "createFakeReplyMsg set destination failed";
    // }

    // const char *error_name = "org.freedesktop.DBus.Error.AccessDenied";
    // ret = dbus_message_set_error_name(dbusMsg, error_name);
    // if (!ret) {
    //     qCritical() << "createFakeReplyMsg set error_name failed";
    // }
    // const char *error_msg = "org.freedesktop.DBus.Error.AccessDenied, please config dbus permission first";

    // dbus_message_set_no_reply(dbusMsg, 1);

    // DBusError dberr;
    // dbus_error_init(&dberr);
    // dbus_error_free(&dberr);
    // dbus_set_error_const(&dberr, error_name, error_msg);
    // char *msgAsc;
    // int len = 0;
    // dbus_message_marshal(dbusMsg, &msgAsc, &len);
    // dbus_message_new_error(DBusMessage * reply_to, const char *error_name, const char *error_message)
    // qInfo() << len;
    // QByteArray reply(msgAsc, len);
    // qInfo() << reply;
    // dbus_free(msgAsc);
    // dbus_message_unref (dbusMsg);

    // QByteArray byteMsg(
    //     "l\x01\x00\x01\x14\x00\x00\x00\x02\x00\x00\x00\x9F\x00\x00\x00\x01\x01o\x00#\x00\x00\x00/com/deepin/linglong/PackageManager\x00\x00\x00\x00\x00\x02\x01s\x00\"\x00\x00\x00"
    //     "com.deepin.linglong.PackageManager\x00\x00\x00\x00\x00\x00\x03\x01s\x00\x04\x00\x00\x00test\x00\x00\x00\x00\x06\x01s\x00\x1E\x00\x00\x00"
    //     "com.deepin.linglong.AppManager\x00\x00\b\x01g\x00\x01s\x00\x00\x0F\x00\x00\x00org.deepin.demo\x00",
    //     196);

    // const char *error_name = "org.freedesktop.DBus.Error.AccessDenied";
    // const char *error_msg = "org.freedesktop.DBus.Error.AccessDenied, please config dbus permission first";
    DBusError dbErr;
    dbus_error_init(&dbErr);
    DBusMessage *receiveMsg = dbus_message_demarshal(byteMsg.constData(), byteMsg.size(), &dbErr);
    if (!receiveMsg) {
        qCritical() << "dbus_message_demarshal failed";
        if (dbus_error_is_set(&dbErr)) {
            qCritical() << "dbus_message_demarshal err msg:" << dbErr.message;
            dbus_error_free(&dbErr);
        }
        return nullptr;
    }

    std::string nameString = errorName.toStdString();
    std::string msgString = errorMsg.toStdString();
    DBusMessage *reply = dbus_message_new_error(receiveMsg, nameString.c_str(), msgString.c_str());
    std::string destination = dst.toStdString();
    auto ret = dbus_message_set_destination(reply, destination.c_str());
    if (!ret) {
        // dbus_message_unref(receiveMsg);
        qCritical() << "createFakeReplyMsg set destination failed";
    }
    dbus_message_set_serial(reply, serial);

    char *replyAsc;
    int len = 0;
    ret = dbus_message_marshal(reply, &replyAsc, &len);
    if (!ret) {
        // dbus_message_unref(receiveMsg);
        qCritical() << "createFakeReplyMsg dbus_message_marshal failed";
    }
    QByteArray data(replyAsc, len);
    dbus_free(replyAsc);
    dbus_message_unref(receiveMsg);
    dbus_message_unref(reply);
    return data;
}