/*
 * libmaeshare.so — MaeShare 的 Maemo 5 / Fremantle 系统分享插件（最小实现）
 *
 * 作用：系统相册（Image Viewer / Photos）里多选照片 → Share → 选 MaeShare →
 *       本插件把选中文件的本地路径通过命令行参数交给 MaeShare 主程序
 *       （/opt/maeshare/maeshare --share <path> <path> ...），主程序自己弹设备选择页。
 *
 * 为什么这样写：
 *   - 设备上没有 libsharing-dev（无头文件），所以这里不 include sharing 头，
 *     只按 libsharing.so 的真实导出符号（nm -D 实测）用 dlsym 取用：
 *        sharing_transfer_get_entry
 *        sharing_entry_get_media
 *        sharing_entry_media_get_localpath
 *     插件必须导出的 7 个入口（与系统自带 libflickr.so / libshareonovi.so 一致）：
 *        sharing_plugin_interface_init / _uninit / _send
 *        _account_setup / _account_validate / _edit_account / _update_options
 *   - 依赖只有 libc + libdl，不链接 glib/gtk，交叉编译不需要额外 -dev 包。
 *
 * 调试日志：/tmp/maeshare-share.log（便于真机排查，正式版可去掉）
 *
 * 源码位置（工程内）：n900/share-int/libmaeshare-plugin.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

#define APP_PATH      "/opt/maeshare/maeshare"
#define LOG_PATH      "/tmp/maeshare-share.log"
/* 系统只把照片转存到 /home/user/.sharing/outbox/attachment-XXXX，
 * 而且 send 返回后立刻删掉，所以必须先拷到自己的固定目录再拉起客户端。
 * 放在家目录下的隐藏目录：不进 MyDocs，图库/媒体索引就不会把副本当成照片扫出来。 */
#define STAGE_DIR       "/home/user/.maeshare-share"
/* 1.0.0 早期版本的暂存目录，里面遗留的副本也要一并清理 */
#define STAGE_DIR_LEGACY "/home/user/MyDocs/.maeshare-share"
/* 超过这个秒数的副本视为孤儿（客户端崩溃、断电等不会来回删） */
#define STAGE_TTL_SEC   (7 * 24 * 3600)
#define MAX_MEDIA     64

/* 系统分享框架的返回码：0 = 成功（与 libflickr / libshareonovi 的用法一致） */
#define SEND_RESULT_SUCCESS 0

/* ---------------- 小工具 ---------------- */

static void logmsg(const char *fmt, ...)
{
    FILE *fp = fopen(LOG_PATH, "a");
    if (!fp) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(fp, fmt, ap);
    va_end(ap);
    fputc('\n', fp);
    fclose(fp);
}

/* 把系统给的临时转存文件拷进自己的目录（附件删掉后客户端就找不到文件了） */
static int copy_file(const char *src, const char *dst)
{
    int in = open(src, O_RDONLY);
    if (in < 0) return -1;
    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0) { close(in); return -1; }

    char buf[8192];
    ssize_t r;
    int ok = 1;
    while ((r = read(in, buf, sizeof(buf))) > 0) {
        char *q = buf;
        while (r > 0) {
            ssize_t w = write(out, q, (size_t)r);
            if (w <= 0) { ok = 0; r = 0; break; }
            q += w; r -= w;
        }
    }
    close(in);
    close(out);
    return ok ? 0 : -1;
}

/* 兜底清理孤儿副本：正常情况下客户端在发送结束就删掉自己的副本，
 * 只有客户端启动失败 / 崩溃 / 断电才会留下，这里按 TTL 清掉。 */
static void sweep_stage_dir(const char *dir, time_t now)
{
    DIR *d = opendir(dir);
    if (!d) return;
    struct dirent *e;
    char path[512];
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        if (strlen(dir) + strlen(e->d_name) + 2 >= sizeof(path)) continue;
        snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
        struct stat st;
        if (stat(path, &st) != 0) continue;
        if (!S_ISREG(st.st_mode)) continue;
        if (now - st.st_mtime > STAGE_TTL_SEC) {
            if (unlink(path) == 0) logmsg("sweep(rm) %s", path);
        }
    }
    closedir(d);
}

static void sweep_stage_dirs(void)
{
    time_t now = time(NULL);
    sweep_stage_dir(STAGE_DIR, now);
    sweep_stage_dir(STAGE_DIR_LEGACY, now);
}

/* 给第 i 个媒体文件生成稳定副本，返回新路径（失败则返回 NULL，调用方退回原路径） */
static char *stage_media(const char *src, int idx)
{
    const char *base = strrchr(src, '/');
    base = base ? base + 1 : src;

    char *dst = (char *)malloc(strlen(STAGE_DIR) + strlen(base) + 24);
    if (!dst) return NULL;
    sprintf(dst, "%s/%d-%d-%s", STAGE_DIR, (int)getpid(), idx, base);

    mkdir(STAGE_DIR, 0700); /* 已存在时返回 EEXIST，忽略即可 */
    if (copy_file(src, dst) != 0) {
        logmsg("stage(fail) %s -> %s : %s", src, dst, strerror(errno));
        free(dst);
        return NULL;
    }
    logmsg("stage(ok) %s", dst);
    return dst;
}

/* libsharing 由宿主（image-viewer / sharing-manager）加载并有全局符号；
 * 这里再 dlopen 一次拿句柄，等价于在全局符号表里查（Maemo 上 RTLD_DEFAULT
 * 需要 _GNU_SOURCE，而该宏会和 glibc 2.5 的 stat64 重定向冲突，故不用）。 */
static void *sym(const char *name)
{
    static void *lib = NULL;
    if (!lib) lib = dlopen("/usr/lib/libsharing.so.0", RTLD_LAZY);
    if (!lib) return NULL;
    return dlsym(lib, name);
}

/*
 * 拉起客户端：execv 继承当前进程的环境（相册进程已经有 DISPLAY / DBUS_SESSION_BUS_ADDRESS，
 * 这正是 Qt 程序能正常出现在前台的条件），参数形态：
 *     /opt/maeshare/maeshare --share <path1> <path2> ...
 * 返回 0 表示已 fork 成功（不阻塞分享面板）。
 */
static int launch_app(char **paths, int count)
{
    char *argv[MAX_MEDIA + 4];
    char *staged[MAX_MEDIA];
    int i, n = 0, staged_count = 0;

    /* 先把系统给的临时文件拷成稳定副本（send 返回后 outbox 里的原件会被删掉） */
    sweep_stage_dirs();   /* 顺手清掉上次留下的孤儿副本 */
    for (i = 0; i < count && i < MAX_MEDIA; ++i) {
        staged[i] = stage_media(paths[i], i);
        if (staged[i]) ++staged_count;
    }

    argv[n++] = (char *)APP_PATH;
    argv[n++] = (char *)"--share";
    for (i = 0; i < count && i < MAX_MEDIA; ++i)
        argv[n++] = staged[i] ? staged[i] : paths[i];
    argv[n] = NULL;

    logmsg("launch(begin) count=%d staged=%d exe=%s", count, staged_count, APP_PATH);
    for (i = 0; i < count; ++i)
        logmsg("  path[%d]=%s", i, argv[2 + i]);

    if (access(APP_PATH, X_OK) != 0) {
        logmsg("launch(fail) %s not executable: %s", APP_PATH, strerror(errno));
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        logmsg("launch(fail) fork: %s", strerror(errno));
        return -1;
    }
    if (pid == 0) {
        /* 子进程：脱离分享面板的会话，避免被一起收走 */
        setsid();
        /* stdio 必须接到 /dev/null：调用方可能是被管道驱动的（ssh / 分享面板），
           子进程攥着继承来的 stdout 会让调用方永远等不到 EOF。 */
        {
            int fd = open("/dev/null", O_RDWR);
            if (fd >= 0) {
                dup2(fd, 0); dup2(fd, 1); dup2(fd, 2);
                if (fd > 2) close(fd);
            }
        }
        execv(APP_PATH, argv);
        logmsg("launch(child) execv failed: %s", strerror(errno));
        _exit(127);
    }

    logmsg("launch(forked) pid=%d", (int)pid);
    return 0;
}

/* ---------------- 插件入口（系统会 dlsym 这 7 个符号） ---------------- */

/*
 * 注意返回值语义（真机反汇编 libsharing.so.0 的 sharing_plugin_load 得到）：
 *   g_module_symbol(... "sharing_plugin_interface_init" ...) → 调用 init(self)
 *   cmp r0,#0 ; beq 成功分支（保留插件）
 *             ; bne 失败分支 → g_module_close() 直接卸载插件
 * 也就是说 **返回 0 才算加载成功**，返回非 0 会被当成失败并把 .so 立刻卸载，
 * 后续 send/account_setup 永远不会被调用（这正是之前"点 MaeShare 没反应"的根因）。
 */
int sharing_plugin_interface_init(void *self)
{
    logmsg("init self=%p", self);
    return 0; /* 0 = 加载成功（Maemo 这里的约定与 GLib 的 TRUE 相反） */
}

void sharing_plugin_interface_uninit(void)
{
    logmsg("uninit");
}

/*
 * 分享面板点 MaeShare 时被调用。
 *   transfer : SharingTransfer*（第 1 参，唯一必需的参数）
 *   connection / dead_mans_switch : 本插件不用，保留占位
 * 取文件路径：transfer -> entry -> GSList(SharingEntryMedia*) -> localpath
 */
int sharing_plugin_interface_send(void *transfer, void *connection, void *dead_mans_switch)
{
    void *(*get_entry)(void *) = (void *(*)(void *))sym("sharing_transfer_get_entry");
    void *(*entry_get_media)(void *) = (void *(*)(void *))sym("sharing_entry_get_media");
    char *(*media_localpath)(void *) = (char *(*)(void *))sym("sharing_entry_media_get_localpath");

    logmsg("send transfer=%p entry_fn=%p media_fn=%p path_fn=%p",
           transfer, get_entry, entry_get_media, media_localpath);

    if (!transfer || !get_entry || !entry_get_media || !media_localpath) {
        logmsg("send(fail) missing sharing symbols");
        return SEND_RESULT_SUCCESS;
    }

    void *entry = get_entry(transfer);
    if (!entry) {
        logmsg("send(fail) entry == NULL");
        return SEND_RESULT_SUCCESS;
    }

    /* GSList 布局：{ gpointer data; GSList *next; } */
    typedef struct _GList { void *data; struct _GList *next; } GList;
    GList *it = (GList *)entry_get_media(entry);

    char *paths[MAX_MEDIA];
    int count = 0;
    while (it && count < MAX_MEDIA) {
        char *p = media_localpath(it->data);
        if (p && p[0] == '/') paths[count++] = p;
        it = it->next;
    }

    logmsg("send collected %d media", count);
    if (count > 0) launch_app(paths, count);

    return SEND_RESULT_SUCCESS;
}

/*
 * 账号由插件自己建（XML 里 plugInSetup="1"）。
 *
 * 真机实测：分享面板发现服务没有账号时，会调用本函数并要求插件"自己把账号建出来"
 * （flickr 就是在这一步弹出自己的登录对话框，成功后调用
 *   sharing_account_store_add_account(store, service, username, password)）。
 *
 * 方案 1（免二次操作）：这里不再只写内存对象，而是直接把账号"建完 + 写全"：
 *   1) store_add_account(store, service, user, pass) -> 拿到 id
 *   2) store_set_account_param(store, id, ...)      -> 把字段写进该 id 的记录
 * 然后返回 0（成功）。目的是让面板认为"账号已就绪"，不再要求用户手动点 Validate。
 */
int sharing_plugin_interface_account_setup(void *svc, void *account, void *b, void *c)
{
    logmsg("account_setup svc=%p account=%p b=%p c=%p", svc, account, b, c);

    /* 框架（sharing_plugin_account_setup）会把参数整体后移一位再调插件，
     * 所以这里第 1 个是 SharingService*，第 2 个才是待填充的 SharingAccount*。
     * 账号的持久化由框架自己做，插件只负责往里写参数（flickr 就是这么干的）。 */
    int (*acc_set_param)(void *, const char *, const char *) =
        (int (*)(void *, const char *, const char *))sym("sharing_account_set_param");
    int (*acc_set_option_values)(void *, void *) =
        (int (*)(void *, void *))sym("sharing_account_set_option_values");

    if (!account || !acc_set_param) {
        logmsg("account_setup(fail) account=%p set_param=%p", account, acc_set_param);
        return 1;
    }

    acc_set_param(account, "username", "maeshare");
    acc_set_param(account, "password", "local");
    acc_set_param(account, "realm", "maeshare");
    acc_set_param(account, "valid", "true");
    logmsg("account_setup params written (username=maeshare)");

    /* 方案 1：自己把账号建完并写全，让面板认为"已就绪"，不再要求手动 Validate */
    void *(*store_get_instance)(void) =
        (void *(*)(void))sym("sharing_account_store_get_instance");
    int (*store_add_account)(void *, void *, const char *, const char *) =
        (int (*)(void *, void *, const char *, const char *))
        sym("sharing_account_store_add_account");
    int (*store_set_param)(void *, int, const char *, const char *) =
        (int (*)(void *, int, const char *, const char *))
        sym("sharing_account_store_set_account_param");

    if (store_get_instance && store_add_account && svc) {
        void *store = store_get_instance();
        if (store) {
            int id = store_add_account(store, svc, "maeshare", "maeshare");
            logmsg("account_setup add_account -> id=%d", id);
            if (id >= 0 && store_set_param) {
                store_set_param(store, id, "username", "maeshare");
                store_set_param(store, id, "password", "maeshare");
                store_set_param(store, id, "realm", "maeshare");
                store_set_param(store, id, "valid", "true");
                logmsg("account_setup store persisted id=%d", id);
            }
        }
    }

    return 0; /* 0 = 成功 */
}

int sharing_plugin_interface_account_validate(void *a, void *b, void *c, void *d)
{
    logmsg("account_validate a=%p b=%p c=%p d=%p", a, b, c, d);

    /* 面板上的 "Validate account to complete account setup" 走的就是这里：
     * 返回非 0 会直接显示 failed，账号永远完不成建号流程。
     * MaeShare 是局域网直连，没有任何凭据要校验，直接返回 0（成功）即可。 */

    int (*acc_set_param)(void *, const char *, const char *) =
        (int (*)(void *, const char *, const char *))sym("sharing_account_set_param");
    if (acc_set_param && a) {
        acc_set_param(a, "username", "maeshare");
        acc_set_param(a, "password", "local");
        acc_set_param(a, "realm", "maeshare");
        acc_set_param(a, "valid", "true");
        logmsg("account_validate ok (marked valid)");
    }
    return 0;
}

int sharing_plugin_interface_edit_account(void *self, void *a, void *b, void *c)
{
    logmsg("edit_account self=%p a=%p b=%p c=%p", self, a, b, c);
    return 0; /* 0 = 成功（无需编辑，直接放行） */
}

int sharing_plugin_interface_update_options(void *self, void *a, void *b, void *c)
{
    logmsg("update_options self=%p a=%p b=%p c=%p", self, a, b, c);
    return 0; /* 0 = 成功（本服务没有可选项） */
}

/*
 * 自测钩子（非系统接口）：dlopen 本插件后调用 maeshare_selftest_launch("a.jpg:b.png")
 * 用来在真机上单独验证「插件 → 客户端」这一段链路是否通。
 */
int maeshare_selftest_launch(const char *csv)
{
    char *buf = strdup(csv ? csv : "");
    char *paths[MAX_MEDIA];
    int count = 0;
    char *save = NULL;
    char *tok = strtok_r(buf, ":", &save);
    while (tok && count < MAX_MEDIA) { paths[count++] = tok; tok = strtok_r(NULL, ":", &save); }
    logmsg("selftest count=%d", count);
    int r = count ? launch_app(paths, count) : -1;
    free(buf);
    return r;
}
