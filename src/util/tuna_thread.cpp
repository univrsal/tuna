/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#include "tuna_thread.hpp"
#include "config.hpp"
#include "../query/music_source.hpp"
#include "../gui/tuna_gui.hpp"
#include "utility.hpp"
#include <util/platform.h>
#include <obs-module.h>
#include <fstream>
#include <QTime>
#include <QFile>
#include <qtextstream.h>

#ifdef LINUX

#include <pthread.h>

#endif
namespace thread
{
	QString song_text;
	volatile bool thread_state = false;
	std::mutex mutex;

#ifdef _WIN32
	static HANDLE thread_handle;
#else
	static pthread_t thread_handle;
#endif

	void write_song(QString& str)
	{
		QFile out(config::song_path);
		if (out.open(QIODevice::WriteOnly | QIODevice::Text)) {
			QTextStream stream(&out);
			stream.setCodec("UTF-8");
			stream << str;
			stream.flush();
			out.close();
		} else {
			blog(LOG_ERROR, "[tuna] Couldn't open song output file %s", config::song_path);
		}
	}

	bool start()
	{
		bool result = true;
		if (thread_state)
			return result;
		thread_state = true;

#ifdef _WIN32
		thread_handle = CreateThread(nullptr, 0,
									 static_cast<LPTHREAD_START_ROUTINE>(thread_method),
									 nullptr, 0, nullptr);
		result = thread_handle;
#else
		result = pthread_create(&thread_handle, nullptr, thread_method, nullptr) == 0;
#endif
		thread_state = result;
		return result;
	}

	void stop()
	{
		thread_state = false;
	}

	QString time_format(uint32_t ms)
	{
		int secs = (ms / 1000) % 60;
		int minute = (ms / 1000) / 60 % 60;
		int hour = (ms / 1000) / 60 / 60 % 60;
		QTime t(hour, minute, secs);

		return t.toString(hour > 0 ? "h:mm:ss" : "m:ss");
	}

	void format_string(QString& out, const song_t* song)
	{
		out = config::format_string;
		out.replace("%t", song->title.c_str());
		out.replace("%m", song->artists.c_str());
		out.replace("%a", song->album.c_str());
		out.replace("%d", QString::number(song->disc_number));
		out.replace("%n", QString::number(song->track_number));
		out.replace("%p", time_format(song->progress_ms));
		out.replace("%l", time_format(song->duration_ms));

		QString full_date = "";
		switch (song->release_precision) {
			case prec_day:
				full_date.append(song->day.c_str()).append('.');
			case prec_month:
				full_date.append(song->month.c_str()).append('.');
			case prec_year:
				full_date.append(song->year.c_str());
			default:;
		}
		out.replace("%y", song->year.c_str());
		out.replace("%r", full_date);
	}

#ifdef _WIN32
	DWORD WINAPI thread_method(LPVOID arg)
#else

	void* thread_method(void*)
#endif
	{
		while (thread_state) {
			mutex.lock();
			if (config::selected_source) {
				config::selected_source->refresh();
				song_text = "";
				auto* s = config::selected_source->song();

				/* Process song data */
				util::handle_cover_art(s);
				util::handle_lyrics(s);

				format_string(song_text, s);
				if (song_text.length() < 1 || !s->is_playing)
					song_text = config::placeholder;

				write_song(song_text);
			}

			mutex.unlock();
			os_sleep_ms(config::refresh_rate);
		}

#ifdef _WIN32
		return 0;
#else
		pthread_exit(nullptr);
#endif
	}
}
