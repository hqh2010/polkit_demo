#pragma once

#include <QDebug>

// add by hqh debug dbus 协议消息头定义
// 参考 Here are the currently-defined header fields:
// The following table summarizes the D-Bus types.
// https://dbus.freedesktop.org/doc/dbus-specification.html#auth-command-auth
typedef struct {
    bool bigEndian;
    uchar type;
    uchar flags;
    quint32 length;
    quint32 serial;
    // const char *path;
    // const char *interface;
    // const char *member;
    // const char *errorName;
    // const char *destination;
    // const char *sender;
    // const char *signature;
    QString path;
    QString interface;
    QString member;
    QString errorName;
    QString destination;
    QString sender;
    QString signature;
    bool hasReplySerial;
    quint32 replySerial;
    quint32 unixFds;
} Header;

enum class MessageType {
    INVALID,
    METHOD_CALL,
    METHOD_RETURN,
    ERROR,
    SIGNAL,
};

enum class DBusMessageHeaderField {
    DBUS_MESSAGE_HEADER_FIELD_INVALID,
    DBUS_MESSAGE_HEADER_FIELD_PATH,
    DBUS_MESSAGE_HEADER_FIELD_INTERFACE,
    DBUS_MESSAGE_HEADER_FIELD_MEMBER,
    DBUS_MESSAGE_HEADER_FIELD_ERROR_NAME,
    DBUS_MESSAGE_HEADER_FIELD_REPLY_SERIAL,
    DBUS_MESSAGE_HEADER_FIELD_DESTINATION,
    DBUS_MESSAGE_HEADER_FIELD_SENDER,
    DBUS_MESSAGE_HEADER_FIELD_SIGNATURE,
    DBUS_MESSAGE_HEADER_FIELD_NUM_UNIX_FDS
};

/*
 * 根据大小端将字节数组转化为整形
 *
 * @param arr: 报文字节数组
 * @param isBigEndian: 是否为大端序
 *
 * @return int: 转化结果
 */
int byteAraryToInt(QByteArray arr, bool isBigEndian);

/*
 * 根据偏移量获取8字节对齐结果
 *
 * @param offset: 偏移量
 *
 * @return quint32: 计算结果
 */
quint32 alignBy8(quint32 offset);

/*
 * 根据偏移量获取4字节对齐结果
 *
 * @param offset: 偏移量
 *
 * @return quint32: 计算结果
 */
quint32 alignBy4(quint32 offset);

/*
 * 从报文中获取指定偏移量的字符数据
 *
 * @param buffer: 报文字节数组
 * @param header: 报文header
 * @param offset: 偏移量开始地址
 * @param endOffset: 偏移量结束地址
 *
 * @return char*: 计算结果
 */
char *getString(QByteArray buffer, Header *header, quint32 *offset, quint32 endOffset);

/*
 * 从报文中获取dbus消息域签名信息
 *
 * @param buffer: 报文字节数组
 * @param offset: 偏移量开始地址
 * @param endOffset: 偏移量结束地址
 *
 * @return char*: 签名结果
 */
char *getSignature(QByteArray buffer, quint32 *offset, quint32 endOffset);

/*
 * 从报文中解析dbus消息报文头
 *
 * @param buffer: 报文字节数组
 * @param header: 偏移量开始地址
 *
 * @return bool: true:解析成功 false:失败
 */
bool parseHeader(QByteArray buffer, Header *header);
