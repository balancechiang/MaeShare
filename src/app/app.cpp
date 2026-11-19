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
#include <QGraphicsItem>   // rootObject() 返回 QGraphicsObject*，需要完整类型才能转 QObject*
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

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/un.h>

/*
 * 早期转交：把 --share 的文件交给已在运行的实例，然后本进程直接退出。
 * 这里刻意不构造 QApplication —— N900 上光是 Qt GUI 初始化就要 30 秒以上，
 * 从相册点「分享」等这么久，看起来就是「拉不起来」。
 * socket 路径必须和 SingleInstance 的 QLocalServer 名字一致：
 *   TMPDIR=/var/tmp + name="chih-localsend-maemo-<uid>"
 */
static bool forwardToRunningInstance(const char **paths, int count)
{
    char sockPath[128];
    struct sockaddr_un addr;
    char line[1200];
    int fd, i;

    snprintf(sockPath, sizeof(sockPath), "/var/tmp/chih-localsend-maemo-%d", (int)getuid());
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return false;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sockPath, sizeof(addr.sun_path) - 1);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(fd);
        return false;          /* 没有实例在跑，走正常启动 */
    }
    for (i = 0; i < count; ++i) {
        snprintf(line, sizeof(line), "share:%s\n", paths[i]);
        if (write(fd, line, strlen(line)) < 0) break;
    }
    write(fd, "activate\n", 9);
    close(fd);
    return true;
}

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

// 客户端被 kill -9 / 崩溃时，probe 子进程会变成孤儿继续占着 UDP 53317，
// 导致新实例的 probe 绑不上端口、界面永远停在 "Looking for devices"。
// 这里只清理「argv[0] 恰好是自己目录下的 probe、且第一个参数是 --serve」的进程：
// 按完整路径比对，绝不会命中同名但不同路径的程序（例如 /opt/n900store-native/probe）。
// 另外先确认没有别的 MaeShare 实例在跑，避免误杀正在服务的那一只 probe。
static void sweepStaleProbe()
{
    char exe[256], probePath[300];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (n <= 0) return;
    exe[n] = 0;
    char *slash = strrchr(exe, '/');
    if (!slash) return;
    *slash = 0;
    snprintf(probePath, sizeof(probePath), "%s/probe", exe);

    DIR *d = opendir("/proc");
    if (!d) return;

    int killed = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] < '0' || e->d_name[0] > '9') continue;
        int pid = atoi(e->d_name);
        if (pid <= 0 || pid == (int)getpid()) continue;

        char path[64], buf[512];
        snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
        FILE *f = fopen(path, "r");
        if (!f) continue;
        size_t len = fread(buf, 1, sizeof(buf) - 1, f);
        fclose(f);
        if (len == 0) continue;
        buf[len] = 0;
        size_t arg0 = strlen(buf);

        /* 还有别的 MaeShare 在主实例状态：什么都不动 */
        if (strcmp(buf, exe) == 0) { closedir(d); return; }
        if (strcmp(buf, probePath) != 0 || len <= arg0 + 1) continue;
        if (strcmp(buf + arg0 + 1, "--serve") != 0) continue;

        if (kill(pid, SIGKILL) == 0) ++killed;
    }
    closedir(d);
    if (killed) qDebug("PROBE_SWEEP killed=%d", killed);
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
    /* 转交时对端读完就关 socket，写第二行会拿到 SIGPIPE；忽略它，改成 ERR/EPIPE 走正常返回 */
    signal(SIGPIPE, SIG_IGN);

    /* ---- 先做参数扫描与「转交给已运行实例」，不碰 Qt GUI ----
     * 只有预览/诊断类参数才走完整启动；--share / --open 是正常入口。 */
    bool earlyDiagnostic = false;
    const char *earlyShare[64];
    int earlyShareCount = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--share") == 0) {
            for (int j = i + 1; j < argc; ++j) {
                if (argv[j][0] == '-' && argv[j][1] == '-') break;
                if (earlyShareCount < 64) earlyShare[earlyShareCount++] = argv[j];
            }
        } else if (argv[i][0] == '-' && argv[i][1] == '-') {
            if (strcmp(argv[i], "--open") != 0) earlyDiagnostic = true;
        }
    }
    if (!earlyDiagnostic && earlyShareCount > 0 &&
        forwardToRunningInstance(earlyShare, earlyShareCount))
        return 0;   /* 已转交，立刻退出（毫秒级），不再拉起第二个界面 */

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));

    // QLocalServer 的 socket 建在 QDir::tempPath() 下：run-standalone.sh 起的实例是
    // /var/tmp，从相册 fork 出来的子进程继承到的可能是 /tmp。两边路径不一致就永远连不上，
    // 各自 listen 变成两个实例（表现为「点 MaeShare 没反应」或冒出第二个窗口）。
    qputenv("TMPDIR", "/var/tmp");

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

    // 只有「预览/诊断」这类参数才跳过单实例。--share / --open 是正常入口：
    // 已有实例在跑时要把文件转交过去，而不是再起一个实例（会撞 UDP 端口、两个窗口）。
    bool diagnostic = false;
    for (int i = 0; i < args.size(); ++i) {
        const QString a = args.at(i);
        if (!a.startsWith("--")) continue;
        if (a == "--share" || a == "--open") continue;
        diagnostic = true;
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

    // 已有实例在跑：连上它、把文件转过去、立刻退出。这里不能建 QDeclarativeView，
    // 否则会白白拉起一个 QGL 窗口（600MHz 上首帧就要十几秒，看起来就像没反应）。
    if (!diagnostic) {
        SingleInstance probe(0, 0);
        if (probe.connectExisting()) {
            if (!sharedPaths.isEmpty()) probe.forwardShare(sharedPaths);
            return 0;
        }
    }

    sweepStaleProbe();   // 上一轮崩溃留下的 probe 会占住 UDP 53317，先清掉再启动

    QDeclarativeView view;
    view.setWindowTitle("MaeShare");
    view.setWindowIcon(QIcon(appDir + "/icon.png"));
    view.setResizeMode(QDeclarativeView::SizeRootObjectToView);
    view.setBackgroundBrush(QColor("#181A1E"));
    view.resize(800, 480);

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
        // 再补一次直接切页：不依赖 shareReady 信号时序，冷启动也一定落在发送页
        if (QObject *root = view.rootObject()) {
            QMetaObject::invokeMethod(root, "goTab", Qt::QueuedConnection,
                                      Q_ARG(QVariant, QVariant(QString("send"))));
        }
        QMetaObject::invokeMethod(&transfer, "scan", Qt::QueuedConnection);
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
