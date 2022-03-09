#include <dbus/dbus.h>
#include <unistd.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDomDocument>

#include "filter/dbus_filter.h"
#include "proxy/dbus_proxy.h"

#include <polkitqt1-authority.h>
using namespace PolkitQt1;

bool checkAuthorization(const QString& actionId, qint64 applicationPid)
{
    Authority::Result result;
    // 第一个参数是需要验证的action，和规则文件写的保持一致
    result = Authority::instance()->checkAuthorizationSync(actionId, UnixProcessSubject(applicationPid),
                                                           Authority::AllowUserInteraction);
    if (result == Authority::Yes) {
        qInfo() << "ttttttttttttttttttttttttt";
        return true;
    }else {
        qInfo() << "fffffffffffffffffffffffff";
        return false;
    }
}

bool setValue(const QString &action)
{
    // This action must be authorized first. It will set the implicit
    // authorization for the Shout action by editing the .policy file
    QDomDocument doc = QDomDocument("policy");
    QFile file("/usr/share/polkit-1/actions/com.polkit.qt.example.policy");
    if (!file.open(QIODevice::ReadOnly))
        return false;
    doc.setContent(&file);
    file.close();
    QDomElement el = doc.firstChildElement("policyconfig").
                     firstChildElement("action");
    while (!el.isNull() && el.attribute("id", QString()) != "org.qt.policykit.examples.shout") {
        el = el.nextSiblingElement("action");
    }
    el = el.firstChildElement("defaults");
    el = el.firstChildElement("allow_active");
    if (el.isNull())
        return false;
    el.firstChild().toText().setData(action);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    QTextStream stream(&file);
    doc.save(stream, 2);
    file.close();
    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    checkAuthorization("com.polkit.qt.example", getpid());

    return app.exec();
}
