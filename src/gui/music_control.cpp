#include "music_control.hpp"
#include "../query/music_source.hpp"
#include "../util/config.hpp"
#include "../util/constants.hpp"
#include "../util/tuna_thread.hpp"
#include "ui_music_control.h"
#include <QDesktopWidget>
#include <QMenu>
#include <QSizePolicy>
#include <QStyle>
#include <obs-frontend-api.h>

class music_control* music_dock = nullptr;

music_control::music_control(QWidget* parent)
    : QDockWidget(parent)
    , ui(new Ui::music_control)
{
    ui->setupUi(this);
    setVisible(false); /* Invisible by default to prevent it from showing until Geometry is loaded */
    const char* geo = CGET_STR(CFG_DOCK_GEOMETRY);
    if (!geo) {
        QByteArray arr = QByteArray::fromBase64(geo);
        restoreGeometry(arr);

        QRect wg = normalGeometry();
        if (!util::window_pos_valid(wg)) {
            QRect r = reinterpret_cast<QApplication*>(obs_frontend_get_main_window())->desktop()->geometry();
            setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), r));
        }
    }

    this->setContextMenuPolicy(Qt::CustomContextMenu);
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), SLOT(refresh_play_state()));

    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showcontextmenu(const QPoint&)));

    /* This is dependent on tuna thread speed so lowering this wouldn't make a difference */
    m_timer->start(700);

    m_song_text = new scroll_text(this);
    m_song_text->setMinimumWidth(200);
    m_song_text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->control_layout->insertWidget(ui->control_layout->count() - 2, m_song_text, 0, Qt::AlignBottom);

    ui->volume_widget->setVisible(CGET_BOOL(CFG_DOCK_VOLUME_VISIBLE));
    m_song_text->setVisible(CGET_BOOL(CFG_DOCK_INFO_VISIBLE));
}

void music_control::save_settings()
{
    if (config::instance) {
        CSET_BOOL(CFG_DOCK_VOLUME_VISIBLE, ui->volume_widget->isVisible());
        CSET_BOOL(CFG_DOCK_INFO_VISIBLE, m_song_text->isVisible());
    }
}

music_control::~music_control()
{
    CSET_BOOL(CFG_DOCK_VISIBLE, isVisible());
    if (isVisible()) {
        CSET_STR(CFG_DOCK_GEOMETRY, saveGeometry().toBase64().constData());
        config_save_safe(config::instance, "tmp", nullptr);
    }

    delete ui;
    delete m_song_text;
}

void music_control::on_btn_prev_clicked()
{
    music_sources::selected_source()->execute_capability(CAP_PREV_SONG);
}

void music_control::on_btn_play_pause_clicked()
{
    music_sources::selected_source()->execute_capability(CAP_PLAY_PAUSE);
}

void music_control::on_btn_next_clicked()
{
    music_sources::selected_source()->execute_capability(CAP_NEXT_SONG);
}

void music_control::refresh_play_state()
{
    static QString last_title = "";
    song copy;

    tuna_thread::copy_mutex.lock();
    copy = tuna_thread::copy;
    tuna_thread::copy_mutex.unlock();
    QString icon = copy.state() == state_playing ? "://images/icons/pause.svg" : "://images/icons/play.svg";
    ui->btn_play_pause->setIcon(QIcon(icon));

    /* refresh song info */
    if (copy.get_string_value('t') != last_title) {
        QString info = utf8_to_qt(T_DOCK_SONG_INFO);
        if (copy.state() <= state_paused) {
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

    refresh_source();
    last_thread_state = tuna_thread::thread_flag;
    setEnabled(tuna_thread::thread_flag);
    save_settings();
}

void music_control::refresh_source()
{
    uint32_t flags = 0;
    if (music_sources::selected_source_unsafe())
        flags = music_sources::selected_source_unsafe()->get_capabilities();

    bool next = flags & CAP_NEXT_SONG, prev = flags & CAP_NEXT_SONG, play = flags & CAP_PLAY_PAUSE,
         stop = flags & CAP_STOP_SONG;

    ui->btn_next->setVisible(next);
    ui->btn_prev->setVisible(prev);
    ui->btn_play_pause->setVisible(play);
    ui->btn_stop->setVisible(stop);

    if (play || stop || next || prev) {
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
}

void music_control::on_btn_stop_clicked()
{
    music_sources::selected_source()->execute_capability(CAP_STOP_SONG);
}

void music_control::showcontextmenu(const QPoint& pos)
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

void music_control::toggle_title()
{
    m_song_text->setVisible(!m_song_text->isVisible());
    save_settings();
}

void music_control::toggle_volume()
{
    auto flags = music_sources::selected_source()->get_capabilities();
    if (flags & CAP_VOLUME_UP || flags & CAP_VOLUME_DOWN)
        ui->volume_widget->setVisible(!ui->volume_widget->isVisible());
    save_settings();
}

void music_control::on_btn_voldown_clicked()
{
    music_sources::selected_source()->execute_capability(CAP_VOLUME_DOWN);
}

void music_control::on_btn_volup_clicked()
{
    music_sources::selected_source()->execute_capability(CAP_VOLUME_UP);
}
