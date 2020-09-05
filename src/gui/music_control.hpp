#pragma once

#include "scrolltext.hpp"
#include <QDockWidget>
#include <QTimer>
#include <memory>

class music_source;

namespace Ui {
class music_control;
}

class music_control : public QDockWidget {
    Q_OBJECT

public:
    explicit music_control(QWidget* parent = nullptr);
    ~music_control();

private slots:
    void refresh_play_state();
    void showcontextmenu(const QPoint& pos);
    void toggle_title();
    void toggle_volume();

    void on_btn_prev_clicked();
    void on_btn_play_pause_clicked();
    void on_btn_next_clicked();
    void on_btn_stop_clicked();
    void on_btn_voldown_clicked();
    void on_btn_volup_clicked();

private:
    void save_settings();
    void refresh_source();
    bool last_thread_state = false;
    Ui::music_control* ui;
    QTimer* m_timer = nullptr;
    scroll_text* m_song_text = nullptr;
};

extern music_control* music_dock;
