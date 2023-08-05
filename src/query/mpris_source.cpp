/**
 ** This file is part of the tuna project.
 ** Copyright 2023 univrsal <uni@vrsal.xyz>.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "mpris_source.hpp"
#include "../gui/widgets/mpris.hpp"
#include "../util/config.hpp"
#include "../util/constants.hpp"
#include "../util/utility.hpp"

/**
 * Large chunks of this source were taken from
 * https://github.com/rmoalic/obs_media_info_plugin
 */

static auto MPRIS_NAME_START = QString("org.mpris.MediaPlayer2");

static inline QString format_name(const char* name)
{

    QString friendly_name = utf8_to_qt(name);
    friendly_name = friendly_name.replace(MPRIS_NAME_START + ".", "");
    friendly_name[0] = friendly_name[0].toUpper();
    return friendly_name;
}

bool mpris_source::init_dbus_session()
{
    DBusConnection* cbus {};
    DBusError error;

    dbus_error_init(&error);
    cbus = dbus_bus_get(DBUS_BUS_SESSION, &error);
    if (dbus_error_is_set(&error)) {
        berr("[MPRIS] Error getting Bus: %s", error.message);
        dbus_error_free(&error);
        return false;
    }
    if (!cbus) {
        berr("[MPRIS] Error getting Bus: cbus is null");
        return false;
    }

    bdebug("DBus name is %s", dbus_bus_get_unique_name(cbus));

    m_dbus_connection = cbus;
    return true;
}

bool mpris_source::dbus_add_matches()
{
    DBusError error;
    dbus_error_init(&error);
    dbus_bus_add_match(m_dbus_connection, "type='signal', interface='org.freedesktop.DBus.Properties',member='PropertiesChanged', path='/org/mpris/MediaPlayer2'", &error);

    if (dbus_error_is_set(&error)) {
        berr("[MPRIS] Error while adding match (%s)", error.message);
        dbus_error_free(&error);
        return false;
    }

    dbus_bus_add_match(m_dbus_connection, "type='signal', interface='org.freedesktop.DBus', member='NameOwnerChanged', path='/org/freedesktop/DBus'", &error);

    if (dbus_error_is_set(&error)) {
        berr("[MPRIS] Error while adding match (%s)", error.message);
        dbus_error_free(&error);
        return false;
    }

    dbus_connection_add_filter(
        m_dbus_connection, [](DBusConnection* connection, DBusMessage* message, void* user_data) {
            return static_cast<mpris_source*>(user_data)->handle_message(connection, message);
        },
        this, dbus_free);
    dbus_connection_flush(m_dbus_connection);
    return true;
}

bool mpris_source::dbus_register_names()
{
    DBusMessage* msg;
    DBusPendingCall* resp_pending = NULL;

    msg = dbus_message_new_method_call("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "ListNames");

    if (!dbus_connection_send_with_reply(m_dbus_connection, msg, &resp_pending, -1)) {
        berr("[MPRIS] Out Of Memory!");
        return false;
    }

    dbus_pending_call_block(resp_pending); // TODO: remove blocking call

    if (dbus_pending_call_get_completed(resp_pending)) {
        DBusMessage* resp;
        resp = dbus_pending_call_steal_reply(resp_pending);
        int current_type;
        DBusMessageIter iter, iter2;

        dbus_message_iter_init(resp, &iter);
        if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
            dbus_message_iter_recurse(&iter, &iter2); // Array of String
            while ((current_type = dbus_message_iter_get_arg_type(&iter2)) != DBUS_TYPE_INVALID) {
                if (current_type == DBUS_TYPE_STRING) {
                    char* name;
                    dbus_message_iter_get_basic(&iter2, &name);
                    if (utf8_to_qt(name).startsWith(MPRIS_NAME_START)) {
                        char* unique_name = dbus_get_name_owner(name);
                        m_players[utf8_to_qt(unique_name)] = format_name(name);
                        bfree(unique_name);
                    }
                }
                dbus_message_iter_next(&iter2);
            }
        }
        dbus_message_unref(resp);
    }
    dbus_message_unref(msg);
    return true;
}

char* mpris_source::dbus_get_name_owner(const char* name)
{
    DBusMessage *msg {}, *resp {};
    DBusMessageIter imsg {};
    DBusError error {};
    char* ret {};
    dbus_error_init(&error);
    msg = dbus_message_new_method_call("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "GetNameOwner");
    dbus_message_iter_init_append(msg, &imsg);
    dbus_message_iter_append_basic(&imsg, DBUS_TYPE_STRING, &name);

    resp = dbus_connection_send_with_reply_and_block(m_dbus_connection, msg, -1, &error); // TODO: remove blocking call
    if (dbus_error_is_set(&error)) {
        berr("[MPRIS] Error while reading owner name (%s)", error.message);
    } else {
        char* resp_name {};

        if (!dbus_message_get_args(resp, &error, DBUS_TYPE_STRING, &resp_name, DBUS_TYPE_INVALID)) {
            if (dbus_error_is_set(&error))
                berr("[MPRIS] Error while reading owner name (%s)", error.message);
        } else {
            ret = bstrdup(resp_name);
        }
        dbus_message_unref(resp);
    }
    dbus_message_unref(msg);
    return ret;
}

DBusHandlerResult mpris_source::handle_dbus(DBusMessage* message)
{
    const char* name;
    const char* old_name;
    const char* new_name;
    DBusError error;
    dbus_error_init(&error);

    if (!dbus_message_get_args(message, &error,
            DBUS_TYPE_STRING, &name,
            DBUS_TYPE_STRING, &old_name,
            DBUS_TYPE_STRING, &new_name,
            DBUS_TYPE_INVALID)) {
        if (dbus_error_is_set(&error)) {
            berr("[MPRIS] Error while reading new names signal (%s)", error.message);
            dbus_error_free(&error);
        }
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    bool registering_name = old_name != NULL && strlen(old_name) == 0;

    if (QString(name).startsWith(MPRIS_NAME_START)) { // if mpris name
        if (registering_name) {
            binfo("[MPRIS] Registration of %s as %s", new_name, name);
            std::lock_guard<std::mutex> lock(m_internal_mutex);
            m_players[utf8_to_qt(new_name)] = format_name(name);
        } else {
            binfo("[MPRIS] Unregistering of %s as %s", old_name, name);
            std::lock_guard<std::mutex> lock(m_internal_mutex);
            m_players.remove(utf8_to_qt(old_name));
        }
    }
    return DBUS_HANDLER_RESULT_HANDLED;
}

static inline QString correct_art_url(const char* url)
{
    return QUrl::toPercentEncoding(utf8_to_qt(url)).replace("%2F", "/").replace("file%3A", "file:"); // idk why it encodes slashes
}

void mpris_source::parse_metadata(DBusMessageIter* iter, QString const& player, int level)
{
    if (level > 20) {
        // WTF???
        berr("MPRIS: Metadata recursion exceeded 20 levels");
        return;
    }

    int current_type {};
    DBusMessageIter sub {}, subsub {};
    const char* property_name {};
    auto has_next = [](auto iter) { return dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_INVALID; };

    while ((current_type = dbus_message_iter_get_arg_type(iter)) != DBUS_TYPE_INVALID) {

        switch (current_type) {
        case DBUS_TYPE_DICT_ENTRY: {
            dbus_message_iter_recurse(iter, &sub);
            parse_metadata(&sub, player, level + 1);

        } break;
        case DBUS_TYPE_STRING: {
            dbus_message_iter_get_basic(iter, &property_name);
        } break;
        case DBUS_TYPE_VARIANT: {
            if (property_name == NULL) {
                berr("[MPRIS] Encountered property before its name, ignoring");
                continue;
            }
            QString prop = utf8_to_qt(property_name);
            if (prop == "xesam:title") {
                char* title;
                dbus_message_iter_recurse(iter, &sub);
                dbus_message_iter_get_basic(&sub, &title);
                m_info[player].metadata.set(meta::TITLE, utf8_to_qt(title));
                m_info[player].update_time = util::epoch();
            } else if (prop == "vlc:length") {
                int length;
                dbus_message_iter_recurse(iter, &sub);
                dbus_message_iter_get_basic(&sub, &length);
                m_info[player].metadata.set(meta::DURATION, length);
                m_info[player].update_time = util::epoch();
            } else if (prop == "mpris:length") {
                int length;
                dbus_message_iter_recurse(iter, &sub);
                dbus_message_iter_get_basic(&sub, &length);
                m_info[player].metadata.set(meta::DURATION, length / 1000);
                m_info[player].update_time = util::epoch();
            } else if (prop == "vlc:publisher") {
                // borked
                //                char* publisher;
                //                dbus_message_iter_recurse(iter, &sub);
                //                dbus_message_iter_recurse(&sub, &subsub);
                //                dbus_message_iter_get_basic(&subsub, &publisher);
                //                m_info[player].metadata.set(meta::PUBLISHER, utf8_to_qt(publisher));
                //                m_info[player].update_time = util::epoch();
            } else if (prop == "mpris:artUrl") {
                char* url;
                QString correct_url;
                dbus_message_iter_recurse(iter, &sub);
                dbus_message_iter_get_basic(&sub, &url);
                correct_url = correct_art_url(url);
                m_info[player].metadata.set(meta::COVER, correct_url);
                m_info[player].update_time = util::epoch();
            } else if (prop == "vlc:copyright") {
                char* copyright;
                dbus_message_iter_recurse(iter, &sub);
                dbus_message_iter_get_basic(&sub, &copyright);
                // close enough
                m_info[player].metadata.set(meta::COPYRIGHT, utf8_to_qt(copyright));
                m_info[player].update_time = util::epoch();
            } else if (prop == "xesam:comment") {
                char* comment;
                dbus_message_iter_recurse(iter, &sub);
                dbus_message_iter_recurse(&sub, &subsub);
                dbus_message_iter_get_basic(&subsub, &comment);
                // close enough
                m_info[player].metadata.set(meta::DESCRIPTION, utf8_to_qt(comment));
                m_info[player].update_time = util::epoch();
            } else if (prop == "xesam:artist") {
                char* artist;
                dbus_message_iter_recurse(iter, &sub);
                if (has_next(&sub)) {
                    dbus_message_iter_recurse(&sub, &subsub);
                    dbus_message_iter_get_basic(&subsub, &artist);
                    m_info[player].metadata.set(meta::ARTIST, QStringList(utf8_to_qt(artist)));
                    m_info[player].update_time = util::epoch();
                } else {
                    dbus_message_iter_get_basic(&sub, &artist);
                    m_info[player].metadata.set(meta::ARTIST, QStringList(utf8_to_qt(artist)));
                    m_info[player].update_time = util::epoch();
                }
            } else if (prop == "xesam:genre") {
                //                char* val;
                //                dbus_message_iter_recurse(iter, &sub);
                //                dbus_message_iter_get_basic(&sub, &val);
                //                m_internal_song.set(meta ::GENRE, QString ::fromUtf8(val));
            } else if (prop == "xesam:album") {
                char* val;
                dbus_message_iter_recurse(iter, &sub);
                dbus_message_iter_get_basic(&sub, &val);
                m_info[player].metadata.set(meta::ALBUM, QString ::fromUtf8(val));
                m_info[player].update_time = util::epoch();
            } else if (prop == "xesam:url") {
                char* val;
                dbus_message_iter_recurse(iter, &sub);
                dbus_message_iter_get_basic(&sub, &val);
                m_info[player].metadata.set(meta::URL, QString ::fromUtf8(val));
                m_info[player].update_time = util::epoch();
            } else if (prop == "xesam:discNumber") {
                int val;
                dbus_message_iter_recurse(iter, &sub);
                dbus_message_iter_get_basic(&sub, &val);
                m_info[player].metadata.set(meta::DISC_NUMBER, val);
                m_info[player].update_time = util::epoch();
            } else if (prop == "xesam:contentCreated") {
                char* val;
                dbus_message_iter_recurse(iter, &sub);
                dbus_message_iter_get_basic(&sub, &val);
                //                m_internal_song.set(meta ::TRACK_NUMBER, val);
            } else if (prop.toLower() == "xesam:tracknumber") {
                int val;
                dbus_message_iter_recurse(iter, &sub);
                dbus_message_iter_get_basic(&sub, &val);
                m_info[player].metadata.set(meta::TRACK_NUMBER, val);
                m_info[player].update_time = util::epoch();
            } else {
                bdebug("[MPRIS] Ignored: %s", property_name);
            }
        } break;
        default:
            berr("[MPRIS] parse error");
        }
        dbus_message_iter_next(iter);
    }
#undef HANDLE
}

bool mpris_source::init_dbus()
{
    return init_dbus_session() && dbus_add_matches() && dbus_register_names();
}

void mpris_source::parse_array(DBusMessageIter* iter, QString const& player, int level)
{
    if (level > 20) {
        // WTF???
        berr("[MPRIS] Metadata recursion exceeded 20 levels");
        return;
    }

    int current_type {};
    DBusMessageIter sub, subsub;
    const char* property_name = NULL;

    while ((current_type = dbus_message_iter_get_arg_type(iter)) != DBUS_TYPE_INVALID) {

        switch (current_type) {
        case DBUS_TYPE_DICT_ENTRY:
            dbus_message_iter_recurse(iter, &sub);
            parse_array(&sub, player, level + 1);
            break;
        case DBUS_TYPE_STRING:
            dbus_message_iter_get_basic(iter, &property_name);
            break;
        case DBUS_TYPE_VARIANT:
            if (property_name == NULL) {
                berr("[MPRIS] Encountered property before its name, ignoring");
                continue;
            }
            if (strcmp(property_name, "PlaybackStatus") == 0) {
                std::lock_guard<std::mutex> lock(m_internal_mutex);
                char* status;
                dbus_message_iter_recurse(iter, &sub);
                dbus_message_iter_get_basic(&sub, &status);
                bdebug("[MPRIS] Status: %s", status);
                if (strcmp(status, "Playing") == 0)
                    m_info[player].metadata.set(meta::STATUS, play_state::state_playing);
                else if (strcmp(status, "Paused") == 0)
                    m_info[player].metadata.set(meta::STATUS, play_state::state_paused);
                else
                    m_info[player].metadata.set(meta::STATUS, play_state::state_stopped);
                m_info[player].update_time = util::epoch();
            } else if (strcmp(property_name, "Metadata") == 0) {
                dbus_message_iter_recurse(iter, &sub);
                dbus_message_iter_recurse(&sub, &subsub);
                parse_metadata(&subsub, player, level + 1);
            } else if (strcmp(property_name, "Position") == 0) {
                std::lock_guard<std::mutex> lock(m_internal_mutex);
                int pos;
                dbus_message_iter_recurse(iter, &sub);
                dbus_message_iter_get_basic(&sub, &pos);
                m_info[player].metadata.set(meta::PROGRESS, pos / 1000);
                m_info[player].update_time = util::epoch();
            } else {
                bdebug("[MPRIS] Not handled %s", property_name);
            }
            break;
        default:
            berr("[MPRIS] Parse error");
        }
        dbus_message_iter_next(iter);
    }
}

DBusHandlerResult mpris_source::handle_mpris(DBusMessage* message)
{
    DBusError error {};
    DBusMessageIter iter {}, sub {};
    int current_type {};

    dbus_error_init(&error);

    const char* property_name;
    auto player = utf8_to_qt(dbus_message_get_sender(message));

    dbus_message_iter_init(message, &iter);
    while ((current_type = dbus_message_iter_get_arg_type(&iter)) != DBUS_TYPE_INVALID) {

        switch (current_type) {
        case DBUS_TYPE_STRING:
            dbus_message_iter_get_basic(&iter, &property_name);
            bdebug("[MPRIS] Dbus type: %s", property_name);
            break;
        case DBUS_TYPE_ARRAY:
            if (strcmp(property_name, "org.mpris.MediaPlayer2.Player") == 0) {
                int c = dbus_message_iter_get_element_count(&iter);
                if (c == 0)
                    break; // if empty array, skip

                dbus_message_iter_recurse(&iter, &sub);
                ensure_entry(player);
                parse_array(&sub, player);
            }
            break;
        default:
            berr("[MPRIS] Failed to parse message");
        }
        dbus_message_iter_next(&iter);
    }

    std::lock_guard<std::mutex> lock(m_internal_mutex);
    if (m_players[player].toLower().contains("vlc")) {

        auto a = m_info[player].metadata;
        auto b = m_prev;
        // Make sure these are equivalent, otherwise if the playing state is the only
        // thing that changed (from playing to paused or stopped) we would then override it again and set it to playing
        b.set(meta::STATUS, play_state::state_unknown);
        a.set(meta::STATUS, play_state::state_unknown);
        if (a != b) {
            m_info[player].metadata.set(meta::STATUS, play_state::state_playing); // if a track changes, it is playing (vlc)
        }
        m_info[player].metadata.set(meta::TRACK_NUMBER, 0);                       // borked on vlc
    }
    return DBUS_HANDLER_RESULT_HANDLED;
}

mpris_source::mpris_source()
    : music_source(S_SOURCE_MPRIS, T_SOURCE_MPRIS, new mpris)
{
    supported_metadata({ meta::ALBUM, meta::TITLE, meta::ARTIST, meta::STATUS, meta::DURATION, meta::DISC_NUMBER, meta::TRACK_NUMBER, meta::PROGRESS, meta::COVER });
    bdebug("[MPRIS] Initialising dbus session for mpris source");
    if (init_dbus()) {
        m_thread_flag = true;
        m_internal_thread = std::thread([](mpris_source* s) {
            util::set_thread_name("tuna-mpris");
            s->internal_refresh();
        },
            this);
    } else {
        berr("[MPRIS] Failed to initialize mpris source");
    }
}

mpris_source::~mpris_source()
{
    m_thread_flag = false;
    m_internal_thread.join();
    //    dbus_connection_close(m_dbus_connection); // apparently you shouldn't close them???
    m_dbus_connection = nullptr;
}

void mpris_source::load()
{
    music_source::load();
    CDEF_STR(CFG_MPRIS_PLAYER, "");
    m_selected_player = utf8_to_qt(CGET_STR(CFG_MPRIS_PLAYER));
}

void mpris_source::refresh()
{
    music_source::begin_refresh();
    std::lock_guard<std::mutex> lock(m_internal_mutex);
    if (m_info.contains(m_selected_player)) {
        m_current = m_info[m_selected_player].metadata;
    } else {
        qint64 most_recent {};
        QString recent_key {};
        for (auto const& p : m_info.keys()) {
            auto const& info = m_info[p];
            if (info.update_time > most_recent) {
                most_recent = info.update_time;
                recent_key = p;
            }
        }

        if (!recent_key.isEmpty()) {
            m_current = m_info[recent_key].metadata;
        }
    }
}

void mpris_source::internal_refresh()
{
    while (m_thread_flag) {
        DBusDispatchStatus remains;

        do {
            if (!dbus_connection_read_write_dispatch(m_dbus_connection, 500)) {
                berr("[MPRIS] Failed to call dbus_connection_read_write_dispatch");
                break;
            }
            remains = dbus_connection_get_dispatch_status(m_dbus_connection);
            if (remains == DBUS_DISPATCH_NEED_MEMORY) {
                bwarn("[MPRIS] Need more memory to read dbus messages");
            }
        } while (remains != DBUS_DISPATCH_COMPLETE);
    }
}

DBusHandlerResult mpris_source::handle_message(DBusConnection* connection, DBusMessage* message)
{
    DBusHandlerResult ret = DBUS_HANDLER_RESULT_HANDLED;

    const char* dest = dbus_message_get_destination(message);
    const char* my_name = dbus_bus_get_unique_name(connection);
    if (dest != NULL && strcmp(dest, my_name) != 0) {
        bwarn("[MPRIS] Received a message for %s I am %s, ignoring", dest, my_name);
        return ret;
    }

    const char* path = dbus_message_get_path(message);
    if (!path)
        return ret;
    const char* member = dbus_message_get_member(message);

    if (strcmp(path, "/org/mpris/MediaPlayer2") == 0) {
        ret = handle_mpris(message);
    } else if (strcmp(path, "/org/freedesktop/DBus") == 0 && strcmp(member, "NameOwnerChanged") == 0) {
        ret = handle_dbus(message);
    } else {
        bwarn("[MPRIS] NOT HANDLED: message with path %s and member %s", path, member);
        ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    return ret;
}
