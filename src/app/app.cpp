// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Chih
// MaeShare for Nokia N900 / Maemo 5 — QML host.
// UI / application layer is implemented for Fremantle in this project (not a port of another app);
// the LocalSend v2 network core lives in ./probe (see probe header for its attribution).
#include <QApplication>
#include <QTextCodec>
#include <QIcon>
#include <QPixmap>
#include <QImage>
#include <QColor>
#include <QStringList>
#include <QTimer>
#include <QCoreApplication>
#include <QUrl>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeError>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include "transfer.h"
#include "singleinstance.h"

// 相册分享的暂存副本由 transfer 在发送结束时删除；只有客户端崩溃 / 断电才会留下。
// 这里启动时按 7 天 TTL 兜底清一次，并跳过本次 --share 传进来的文件。
static void sweepShareStaging(const QStringList &keep) {
    QStringList dirs;
    dirs << QDir::homePath() + "/.maeshare-share"
         << QDir::homePath() + "/MyDocs/.maeshare-share";
    QStringList keepCanonical;
    for (int i = 0; i < keep.size(); ++i) {
        QString c = QFileInfo(keep.at(i)).canonicalFilePath();
        if (!c.isEmpty()) keepCanonical << c;
    }
    const qint64 ttl = (qint64)7 * 24 * 3600;
    QDateTime now = QDateTime::currentDateTime();
    for (int i = 0; i < dirs.size(); ++i) {
        QDir dir(dirs.at(i));
        if (!dir.exists()) continue;
        QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
        for (int j = 0; j < list.size(); ++j) {
            QString path = list.at(j).canonicalFilePath();
            if (path.isEmpty() || keepCanonical.contains(path)) continue;
            if (list.at(j).lastModified().secsTo(now) > ttl) QFile::remove(path);
        }
    }
}

static int previewTabIndex(const QStringList &args) {
    // 相册「分享」进来时直接落在发送页（已选内容 + 设备列表）
    if (args.contains("--share")) return 1;
    if (args.contains("--tab-send")) return 1;
    if (args.contains("--tab-language")) return 12;
    if (args.contains("--tab-settings")) return 2;
    if (args.contains("--tab-name")) return 3;
    if (args.contains("--tab-about")) return 5;
    if (args.contains("--tab-photos")) return 6;
    if (args.contains("--tab-folder")) return 7;
    if (args.contains("--tab-notice")) return 8;
    if (args.contains("--tab-files")) return 9;
    if (args.contains("--tab-offer")) return 10;
    if (args.contains("--tab-pics")) return 11;
    if (args.contains("--tab-text")) return 14;
    if (args.contains("--tab-textsend")) return 15;
    if (args.contains("--tab-history")) return 16;
    if (args.contains("--tab-info")) return 17;
    if (args.contains("--tab-picked")) return 18;
    if (args.contains("--tab-picked-send")) return 19;
    if (args.contains("--tab-revisit")) return 20;
    if (args.contains("--tab-devices")) return 21;
    if (args.contains("--tab-manual")) return 22;
    if (args.contains("--tab-manual-connect")) return 23;
    if (args.contains("--tab-manual-send")) return 24;
    return 0;
}

int main(int argc, char **argv) {
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));

    // N900 的 SGX530 上 Qt 的 opengl 后端可用（日志会打印 Found SGX/MBX driver），
    // 让界面动画走 GPU；出问题可用 MAESHARE_RASTER=1 强制回软件光栅。
    // 必须在 QApplication 构造之前设置，才可以生效。
    if (qgetenv("QT_GRAPHICSSYSTEM").isEmpty() && qgetenv("MAESHARE_RASTER").isEmpty())
        qputenv("QT_GRAPHICSSYSTEM", "opengl");

    QApplication app(argc, argv);
    // 保留旧的 QSettings 组织/应用名，避免升级后丢失自动保存、目录等设置
    app.setApplicationName("MaeShare");
    app.setOrganizationName("Chih");

    QStringList args = app.arguments();
    QString appDir = QCoreApplication::applicationDirPath();
    QString qmlPath = appDir + "/Main.qml";
    for (int i = 0; i < args.size(); ++i) {
        if (args.at(i) == "--qml" && i + 1 < args.size()) qmlPath = args.at(i + 1);
    }

    QDeclarativeView view;
    view.setWindowTitle("MaeShare");
    view.setWindowIcon(QIcon(appDir + "/icon.png"));
    view.setResizeMode(QDeclarativeView::SizeRootObjectToView);
    view.setBackgroundBrush(QColor("#181A1E"));
    view.resize(800, 480);

    bool diagnostic = false;
    for (int i = 0; i < args.size(); ++i) {
        if (args.at(i).startsWith("--")) diagnostic = true;
    }

    // 相册分享：--share 后面跟若干个文件绝对路径（libsharing 插件就是这么调的）
    QStringList sharedPaths;
    for (int i = 0; i < args.size(); ++i) {
        if (args.at(i) != "--share") continue;
        for (int j = i + 1; j < args.size(); ++j) {
            if (args.at(j).startsWith("--")) break;
            sharedPaths.append(args.at(j));
        }
        break;
    }

    Transfer transfer;
    SingleInstance instance(&view, &transfer);
    if (!diagnostic && !instance.acquire()) {
        // 已经有实例在跑：把文件列表转过去，然后自己退出
        if (!sharedPaths.isEmpty()) instance.forwardShare(sharedPaths);
        return 0;
    }
    if (!diagnostic) sweepShareStaging(sharedPaths);   // 主实例启动时清一次孤儿副本

    view.rootContext()->setContextProperty("transfer", &transfer);
    view.rootContext()->setContextProperty("previewTab", previewTabIndex(args));
    QObject::connect(view.engine(), SIGNAL(quit()), &app, SLOT(quit()));

    QUrl url = QUrl::fromLocalFile(qmlPath);
    view.setSource(url);
    if (view.status() == QDeclarativeView::Error) {
        QList<QDeclarativeError> errors = view.errors();
        for (int i = 0; i < errors.size(); ++i) {
            fprintf(stderr, "QML_ERROR %s\n", errors.at(i).toString().toUtf8().constData());
        }
        return 1;
    }

    // 普通窗口：保留 Hildon 原生顶栏与任务切换，可随时切到后台/其他应用
    view.show();

    // 相册分享进来的文件：在 QML 起来之后写入「已选内容」，发送页会直接列出设备
    if (!sharedPaths.isEmpty()) {
        QMetaObject::invokeMethod(&transfer, "shareFiles", Qt::QueuedConnection,
                                  Q_ARG(QStringList, sharedPaths));
    }

    // 切到后台时停掉接收页的波纹动画：Qt Quick 1 的动画是持续重绘整帧，
    // 600MHz 的机器上开着就是白烧 CPU（实测前台 ~22% 一路不停）。
    class ActiveFilter : public QObject {
    public:
        Transfer *t;
        ActiveFilter(Transfer *tr) : t(tr) {}
        bool eventFilter(QObject *, QEvent *e) {
            if (e->type() == QEvent::WindowActivate) t->setActive(true);
            else if (e->type() == QEvent::WindowDeactivate) t->setActive(false);
            return false;
        }
    };
    ActiveFilter activeFilter(&transfer);
    view.installEventFilter(&activeFilter);

    // 诊断用：--open <file> 直接走一次 openFile，用来在真机上验证「点历史条目打开文件」
    // 这条链路到底走不走得通（日志 OPEN_TRY / OPEN_OK / OPEN_FAIL）。
    QString openPath;
    for (int i = 0; i < args.size(); ++i) {
        if (args.at(i) == "--open" && i + 1 < args.size()) openPath = args.at(i + 1);
    }
    if (!openPath.isEmpty()) {
        transfer.setPendingOpen(openPath);
        QTimer::singleShot(1500, &transfer, SLOT(openPendingFile()));
    }

    if (args.contains("--preview")) {
        int tab = previewTabIndex(args);
        // 截图前多等几秒：设备发现靠 UDP 广播，太早抓图会拍到空列表；
        // 另外这台 600MHz 机器的 QML 首帧要 10 秒以上，6 秒会拍到空窗口。
        QTimer::singleShot(16000, &app, SLOT(quit()));
        int code = app.exec();
        QImage capture = QPixmap::grabWidget(&view).toImage();
        QString out = appDir + "/preview-" + QString::number(tab) + ".png";
        capture.save(out);
        fprintf(stderr, "PREVIEW_SAVED %s %dx%d\n", out.toUtf8().constData(), capture.width(), capture.height());
        return code;
    }

    return app.exec();
}
