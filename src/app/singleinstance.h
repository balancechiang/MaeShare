// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Chih
#ifndef N9_SINGLE_INSTANCE_H
#define N9_SINGLE_INSTANCE_H
#include <QWidget>
#include <QLocalServer>
#include <QLocalSocket>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <unistd.h>

// 用 QObject* 而不是 Transfer*：这里只需要能 invokeMethod，
// 前向声明真正的 Q_OBJECT 类型会拿不到 QObject 基类（moc 编译期报错）。
class SingleInstance : public QObject {
    Q_OBJECT
    QLocalServer server;
    QWidget *window;
    QObject *transfer;
    QString name;

    bool activateExisting() {
        QLocalSocket socket;
        socket.connectToServer(name);
        if (!socket.waitForConnected(200)) return false;
        socket.write("activate\n");
        socket.waitForBytesWritten(200);
        socket.disconnectFromServer();
        return true;
    }

public:
    SingleInstance(QWidget *view, QObject *tr = 0)
        : window(view), transfer(tr), name("chih-localsend-maemo-" + QString::number(getuid())) {
        connect(&server, SIGNAL(newConnection()), this, SLOT(activate()));
    }

    bool acquire() {
        if (activateExisting()) return false;
        if (server.listen(name)) return true;
        if (activateExisting()) return false;
        if (server.serverError() == QAbstractSocket::AddressInUseError) {
            QLocalServer::removeServer(name);
            return server.listen(name);
        }
        return false;
    }

    // 把「相册分享」的文件列表转给已在运行的实例；成功返回 true
    bool forwardShare(const QStringList &paths) {
        QLocalSocket socket;
        socket.connectToServer(name);
        if (!socket.waitForConnected(200)) return false;
        for (int i = 0; i < paths.size(); ++i)
            socket.write(QString("share:%1\n").arg(paths.at(i)).toUtf8());
        socket.write("activate\n");
        socket.waitForBytesWritten(300);
        socket.disconnectFromServer();
        return true;
    }

private slots:
    void activate() {
        while (server.hasPendingConnections()) {
            QLocalSocket *socket = server.nextPendingConnection();
            if (socket->waitForReadyRead(150))
                handlePayload(socket->readAll());
            else
                handlePayload(QByteArray());
            socket->close();
            socket->deleteLater();
        }
        if (window) {
            // 普通窗口：不抢全屏，保留 Hildon 原生顶栏与后台切换能力
            window->show();
            window->raise();
            window->activateWindow();
        }
    }

private:
    void handlePayload(const QByteArray &data) {
        if (!transfer || data.isEmpty()) return;
        QStringList paths;
        QStringList lines = QString::fromUtf8(data).split('\n', QString::SkipEmptyParts);
        for (int i = 0; i < lines.size(); ++i) {
            QString line = lines.at(i).trimmed();
            if (line.startsWith("share:")) paths.append(line.mid(6));
        }
        if (!paths.isEmpty())
            QMetaObject::invokeMethod(transfer, "shareFiles", Qt::QueuedConnection,
                                      Q_ARG(QStringList, paths));
    }
};
#endif
