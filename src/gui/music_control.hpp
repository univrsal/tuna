#pragma once

#include "scrolltext.hpp"
#include <QDockWidget>
#include <QTimer>

namespace Ui {
class music_Control;
}

class music_Control : public QDockWidget {
    Q_OBJECT
    bool m_visible = false; /* I don't know any other way to keep track of this */

public:
    explicit music_Control(QWidget* parent = nullptr);
    ~music_Control();

    void save_settings();
    bool visible()
    {
        return m_visible;
    }

    virtual void closeEvent(QCloseEvent* event);
signals:
    void source_changed();
    void thread_changed();

private Q_SLOTS:
    void on_source_changed();
    void on_thread_changed();
    void refresh_play_state();
    void showcontextmenu(const QPoint& pos);
    void toggle_title();
    void toggle_volume();
private slots:

    void on_btn_prev_clicked();

    void on_btn_play_pause_clicked();

    void on_btn_next_clicked();

    void on_btn_stop_clicked();

    void on_btn_voldown_clicked();

    void on_btn_volup_clicked();

private:
    Ui::music_Control* ui;
    QTimer* m_timer = nullptr;
    scroll_text* m_song_text = nullptr;
};

extern music_Control* music_control;
