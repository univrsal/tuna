#include "music_control.hpp"
#include "../query/music_source.hpp"
#include "../util/config.hpp"
#include "../util/constants.hpp"
#include "../util/tuna_thread.hpp"
#include "ui_music_control.h"
#include <obs-frontend-api.h>
#include <QMenu>
#include <QStyle>
#include <QDesktopWidget>

music_Control *music_control = nullptr;

music_Control::music_Control(QWidget *parent) : QDockWidget(parent), ui(new Ui::music_Control)
{
    ui->setupUi(this);

    const char *geo = CGET_STR(CFG_DOCK_GEOMETRY);
    if (!geo) {
        QByteArray arr = QByteArray::fromBase64(geo);
        restoreGeometry(arr);

        QRect wg = normalGeometry();
        if (!util::window_pos_valid(wg)) {
            QRect r = reinterpret_cast<QApplication *>(
                          obs_frontend_get_main_window())->desktop()->geometry();
            setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter,
                                            size(), r));
        }
    }

    this->setContextMenuPolicy(Qt::CustomContextMenu);
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), SLOT(refresh_play_state()));

    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showcontextmenu(const QPoint &)));
    connect(this, &music_Control::source_changed, this, &music_Control::on_source_changed);
    connect(this, &music_Control::thread_changed, this, &music_Control::on_thread_changed);

    /* This is dependent on tuna thread speed so lowering this wouldn't make a difference */
    m_timer->start(500);
    /* Wait for config to load and then check update dock according to selected source */
    QTimer::singleShot(3000, this, SLOT(on_source_changed()));
    QTimer::singleShot(3000, this, SLOT(on_thread_changed()));

    m_song_text = new scroll_text(this);
    ui->control_layout->addWidget(m_song_text, Qt::AlignBottom);

    if (!CGET_BOOL(CFG_DOCK_INFO_VISIBLE))
        setVisible(false);
    ui->volume_widget->setVisible(CGET_BOOL(CFG_DOCK_VOLUME_VISIBLE));
    m_song_text->setVisible(CGET_BOOL(CFG_DOCK_INFO_VISIBLE));
}

void music_Control::closeEvent(QCloseEvent *event)
{
    CSET_BOOL(CFG_DOCK_VISIBLE, isVisible());
    if (isVisible()) {
        CSET_STR(CFG_DOCK_GEOMETRY, saveGeometry().toBase64().constData());
        config_save_safe(config::instance, "tmp", nullptr);
    }
}

void music_Control::save_settings()
{
    if (config::instance) {
        CSET_BOOL(CFG_DOCK_VOLUME_VISIBLE, ui->volume_widget->isVisible());
        CSET_BOOL(CFG_DOCK_INFO_VISIBLE, m_song_text->isVisible());
    }
}

music_Control::~music_Control()
{
    delete ui;
    delete m_song_text;
}

void music_Control::on_btn_prev_clicked()
{
    thread::thread_mutex.lock();
    music_sources::selected_source()->execute_capability(CAP_PREV_SONG);
    thread::thread_mutex.unlock();
}

void music_Control::on_btn_play_pause_clicked()
{
    thread::thread_mutex.lock();
    music_sources::selected_source()->execute_capability(CAP_PLAY_PAUSE);
    thread::thread_mutex.unlock();
}

void music_Control::on_btn_next_clicked()
{
    thread::thread_mutex.lock();
    music_sources::selected_source()->execute_capability(CAP_NEXT_SONG);
    thread::thread_mutex.unlock();
}

void music_Control::refresh_play_state()
{
    static QString last_title = "";
    song copy;
    thread::copy_mutex.lock();
    copy = thread::copy;
    thread::copy_mutex.unlock();
    QString icon = copy.playing() ? "://images/icons/pause.svg" : "://images/icons/play.svg";
    ui->btn_play_pause->setIcon(QIcon(icon));

    /* refresh song info */
    if (copy.get_string_value('t') != last_title) {
        QString info = utf8_to_qt(T_DOCK_SONG_INFO);
        if (copy.playing()) {
            last_title = copy.get_string_value('t');
            QString artists, title = copy.get_string_value('t');

            artists = copy.artists().join(", ");
            info.append(artists);
            info.append(" - ").append(title);
            last_title = title;
        } else {
            info.append(utf8_to_qt(config::placeholder));
            last_title = "n/a";
        }
        info.replace("%s", " ");
        m_song_text->set_text(info);
    }
}

void music_Control::on_source_changed()
{
    thread::thread_mutex.lock();
    auto flags = music_sources::selected_source()->get_capabilities();
    thread::thread_mutex.unlock();

    ui->btn_next->setVisible(flags & CAP_NEXT_SONG);
    ui->btn_prev->setVisible(flags & CAP_PREV_SONG);
    ui->btn_play_pause->setVisible(flags & CAP_PLAY_PAUSE);
    ui->btn_stop->setVisible(flags & CAP_STOP_SONG);

    if (ui->btn_next->isVisible() || ui->btn_prev->isVisible() || ui->btn_stop->isVisible() ||
        ui->btn_play_pause->isVisible()) {
        ui->control_widget->setVisible(true);
    } else {
        ui->control_widget->setVisible(false);
    }

    ui->btn_volup->setVisible(flags & CAP_VOLUME_UP);
    ui->btn_voldown->setVisible(flags & CAP_VOLUME_DOWN);

    if (ui->btn_volup->isVisible() || ui->btn_voldown->isVisible()) {
        ui->volume_widget->setVisible(true);
    } else {
        ui->volume_widget->setVisible(false);
    }
    save_settings();
}

void music_Control::on_thread_changed()
{
    thread::thread_mutex.lock();
    auto state = thread::thread_running;
    thread::thread_mutex.unlock();
    setEnabled(state);
}

void music_Control::on_btn_stop_clicked()
{
    thread::thread_mutex.lock();
    music_sources::selected_source()->execute_capability(CAP_STOP_SONG);
    thread::thread_mutex.unlock();
}

void music_Control::showcontextmenu(const QPoint &pos)
{
    QMenu contextMenu(T_DOCK_MENU_TITLE, this);

    QAction title(T_DOCK_TOGGLE_INFO, this);
    QAction volume(T_DOCK_TOGGLE_VOLUME, this);

    connect(&title, SIGNAL(triggered()), this, SLOT(toggle_title()));
    connect(&volume, SIGNAL(triggered()), this, SLOT(toggle_volume()));

    contextMenu.addAction(&title);
    contextMenu.addAction(&volume);

    contextMenu.exec(mapToGlobal(pos));
}

void music_Control::toggle_title()
{
    m_song_text->setVisible(!m_song_text->isVisible());
    save_settings();
}

void music_Control::toggle_volume()
{
    thread::thread_mutex.lock();
    auto flags = music_sources::selected_source()->get_capabilities();
    thread::thread_mutex.unlock();
    if (flags & CAP_VOLUME_UP || flags & CAP_VOLUME_DOWN)
        ui->volume_widget->setVisible(!ui->volume_widget->isVisible());
    save_settings();
}

void music_Control::on_btn_voldown_clicked()
{
    thread::thread_mutex.lock();
    music_sources::selected_source()->execute_capability(CAP_VOLUME_DOWN);
    thread::thread_mutex.unlock();
}

void music_Control::on_btn_volup_clicked()
{
    thread::thread_mutex.lock();
    music_sources::selected_source()->execute_capability(CAP_VOLUME_UP);
    thread::thread_mutex.unlock();
}
