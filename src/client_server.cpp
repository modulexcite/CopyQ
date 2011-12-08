#include "client_server.h"
#include <iostream>
#include <QObject>
#include <QThread>
#include <unistd.h>
#include <cstdio>

// msleep function (portable)
class Sleeper : public QThread
{
public:
    static void msleep(unsigned long msecs)
    {
        QThread::msleep(msecs);
    }
};

void log(const QString &text, const LogLevel level)
{
    const char *msg;
    QString level_id;

    if (level == LogNote)
        level_id = QObject::tr("CopyQ: %1\n");
    else if (level == LogWarning)
        level_id = QObject::tr("CopyQ warning: %1\n");
    else if (level == LogError)
        level_id = QObject::tr("CopyQ ERROR: %1\n");

    msg = level_id.arg(text).toLocal8Bit().constData();
    fprintf(stderr, msg);
}

bool waitForBytes(QIODevice *socket, qint64 size) {
    bool res = true;
    while( res && socket->bytesAvailable() < size )
        res = socket->waitForReadyRead(1000);
    return res;
}

bool readBytes(QIODevice *socket, QByteArray &msg)
{
    QDataStream in(socket);

    quint32 sz;
    if( !waitForBytes(socket, (qint64)sizeof(quint32)) )
        return false;
    in >> sz;

    if( !waitForBytes(socket, sz) )
        return false;
    in >> msg;

    return true;
}

QLocalServer *newServer(const QString &name, QObject *parent)
{
    QLocalServer *server = new QLocalServer(parent);

    if ( !server->listen(name) ) {
        QLocalSocket socket;
        socket.connectToServer(name);
        if ( socket.waitForConnected(2000) ) {
            QDataStream out(&socket);
            out << (quint32)0;
        } else {
            // server is not running but socket is open -> remove socket
            QLocalServer::removeServer(name);
            server->listen(name);
        }
    }

    return server;
}

QString serverName(const QString &name)
{
    return name + QString::number(getuid(), 16);
}

QMimeData *cloneData(const QMimeData &data, const QStringList *formats)
{
    QMimeData *newdata = new QMimeData;
    if (formats) {
        for(int i=0; i<formats->length(); ++i) {
            const QString &mime = formats->at(i);
            QByteArray bytes = data.data(mime);
            if ( !bytes.isEmpty() )
                newdata->setData(mime, bytes);
        }
    } else {
        foreach( QString mime, data.formats() ) {
            // ignore uppercase mimetypes (e.g. UTF8_STRING, TARGETS, TIMESTAMP)
            if ( mime[0].isLower() )
                newdata->setData(mime, data.data(mime));
        }
    }
    return newdata;
}
