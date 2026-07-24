/* tinympris.h -- v1.0 -- stb-style single-header MPRIS (Media Player Remote Interfacing
 * Specification) client for C/C++.
 *
 * Connects to the D-Bus session bus, tracks every running MPRIS player
 * (org.mpris.MediaPlayer2.*), and hands metadata/playback-state changes to the caller
 * through a single callback.
 *
 * Reference: https://specifications.freedesktop.org/mpris/latest/Player_Interface.html
 *
 * USAGE
 * -----
 *   #define TINYMPRIS_IMPLEMENTATION
 *   #include "tinympris.h"
 *
 * Do this in exactly one C or C++ translation unit. Everywhere else, just
 * `#include "tinympris.h"` without the define.
 *
 * DEPENDENCIES
 * ------------
 * Requires libdbus (dbus/dbus.h, link with -ldbus-1). Session-bus based, so this only
 * works on Linux/BSD style systems that run a D-Bus session daemon.
 *
 * MODEL
 * -----
 * This library is poll-based and single-threaded: it does not spawn any threads or take
 * any locks. Call tinympris_process() periodically (e.g. once per frame, or in a loop with
 * a timeout) from whichever thread owns the tinympris_ctx. The callback you pass to
 * tinympris_init() is invoked synchronously, from inside tinympris_process(), whenever a
 * player appears, disappears, or has its metadata/playback-state change.
 *
 * The `tinympris_player*` passed to the callback is owned by the library and only valid
 * for the duration of that call; copy any fields you need to keep afterwards.
 * 
 * LICENSE
 * -------
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 * 
 */

/* Needed for clock_gettime()/CLOCK_MONOTONIC (used internally for Position polling) under
 * strict standards modes (-std=c11, etc). Must be defined before any system header is
 * included anywhere in the translation unit, including the ones just below, which is why
 * this appears ahead of the include guard. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#ifndef TINYMPRIS_H
#define TINYMPRIS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tinympris_playback_status {
    TINYMPRIS_STATUS_UNKNOWN = 0,
    TINYMPRIS_STATUS_PLAYING,
    TINYMPRIS_STATUS_PAUSED,
    TINYMPRIS_STATUS_STOPPED
} tinympris_playback_status;

typedef enum tinympris_loop_status {
    TINYMPRIS_LOOP_UNKNOWN = 0,
    TINYMPRIS_LOOP_NONE,
    TINYMPRIS_LOOP_TRACK,
    TINYMPRIS_LOOP_PLAYLIST
} tinympris_loop_status;

typedef enum tinympris_event_type {
    TINYMPRIS_EVENT_PLAYER_ADDED = 0,
    TINYMPRIS_EVENT_PLAYER_REMOVED,
    TINYMPRIS_EVENT_PLAYER_UPDATED
} tinympris_event_type;

/* Mirrors the well known keys of the MPRIS Metadata_Map (xesam:*, mpris:*). Any field may be
 * left at its zero value if the player did not report it. */
typedef struct tinympris_metadata {
    char*   track_id;     /* mpris:trackid, a D-Bus object path, may be NULL */
    int64_t length_us;    /* mpris:length, microseconds, 0 if unknown */
    char*   art_url;      /* mpris:artUrl; file:// urls are percent-decoded, others left as-is */
    char*   album;        /* xesam:album, may be NULL */
    char*   title;        /* xesam:title, may be NULL */
    char*   url;          /* xesam:url, may be NULL */
    char**  artists;      /* xesam:artist, one or more artist names */
    size_t  artist_count;
    int     track_number; /* xesam:trackNumber, 0 if unknown */
    int     disc_number;  /* xesam:discNumber, 0 if unknown */
} tinympris_metadata;

typedef struct tinympris_player {
    char* bus_name;        /* unique connection name, e.g. ":1.23" */
    char* well_known_name;  /* e.g. "org.mpris.MediaPlayer2.vlc" */
    char* identity;         /* friendly name derived from well_known_name, e.g. "Vlc" */

    tinympris_playback_status status;
    tinympris_loop_status     loop_status;
    bool    shuffle;
    double  volume;
    double  rate;
    int64_t position_us;

    bool can_go_next;
    bool can_go_previous;
    bool can_play;
    bool can_pause;
    bool can_seek;
    bool can_control;

    tinympris_metadata metadata;
} tinympris_player;

typedef struct tinympris_ctx tinympris_ctx;

/* Invoked for every player add/remove/update. `player` is owned by the library and only
 * valid for the duration of this call - copy any strings you need to keep beyond that. */
typedef void (*tinympris_callback)(tinympris_event_type event, const tinympris_player* player, void* user_data);

/* Connects to the D-Bus session bus, subscribes to the relevant MPRIS signals, and
 * enumerates the players that are already running (firing TINYMPRIS_EVENT_PLAYER_ADDED
 * once per player found). Returns NULL if no session bus is available. */
tinympris_ctx* tinympris_init(tinympris_callback cb, void* user_data);

/* Pumps the D-Bus connection for up to timeout_ms milliseconds, dispatching any pending
 * messages. The callback may fire zero or more times from within this call. Call this
 * repeatedly (e.g. in a loop) to keep player state up to date. Returns 0 on success, -1 if
 * the connection has been lost. */
int tinympris_process(tinympris_ctx* ctx, int timeout_ms);

/* Frees all tracked players and the context. Does not close the underlying D-Bus
 * connection: it is a shared connection owned by libdbus (dbus_bus_get), and closing
 * shared connections is not appropriate. */
void tinympris_shutdown(tinympris_ctx* ctx);

size_t                  tinympris_get_player_count(const tinympris_ctx* ctx);
const tinympris_player* tinympris_get_player_at(const tinympris_ctx* ctx, size_t index);
/* `name` may be either the unique bus name (":1.23") or the well known name
 * (org.mpris.MediaPlayer2.foo). */
const tinympris_player* tinympris_get_player_by_name(const tinympris_ctx* ctx, const char* name);

/* Sets how often (in milliseconds) the library actively polls the "Position" property of
 * each currently Playing player. Per the MPRIS spec, PropertiesChanged is NOT emitted when
 * Position changes during normal playback, so periodic polling is required to keep it
 * up to date; Seeked signals still update it immediately in between polls. Default is
 * 1000ms. Pass 0 to disable periodic polling entirely. */
void tinympris_set_position_poll_interval_ms(tinympris_ctx* ctx, int interval_ms);

/* Basic playback control, matching the MPRIS Player interface methods. `player_name` may be
 * the unique bus name or the well known name of a currently tracked player. These are
 * "fire and forget": they do not wait for the method call to complete. Return 0 on success,
 * -1 if the player is not currently tracked or the message could not be sent. */
int tinympris_play(tinympris_ctx* ctx, const char* player_name);
int tinympris_pause(tinympris_ctx* ctx, const char* player_name);
int tinympris_play_pause(tinympris_ctx* ctx, const char* player_name);
int tinympris_stop(tinympris_ctx* ctx, const char* player_name);
int tinympris_next(tinympris_ctx* ctx, const char* player_name);
int tinympris_previous(tinympris_ctx* ctx, const char* player_name);
int tinympris_seek(tinympris_ctx* ctx, const char* player_name, int64_t offset_us);
int tinympris_set_position(tinympris_ctx* ctx, const char* player_name, const char* track_id, int64_t position_us);
int tinympris_open_uri(tinympris_ctx* ctx, const char* player_name, const char* uri);

#ifdef __cplusplus
}
#endif

#endif /* TINYMPRIS_H */

/* =============================================================================================
 * IMPLEMENTATION
 * ============================================================================================= */
#ifdef TINYMPRIS_IMPLEMENTATION

#include <dbus/dbus.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TINYMPRIS_NAME_PREFIX  "org.mpris.MediaPlayer2"
#define TINYMPRIS_PLAYER_IFACE "org.mpris.MediaPlayer2.Player"
#define TINYMPRIS_PATH         "/org/mpris/MediaPlayer2"

struct tinympris_ctx {
    DBusConnection*    conn;
    tinympris_callback cb;
    void*              user_data;
    tinympris_player**  players;
    size_t             count;
    size_t             capacity;
    int                position_poll_interval_ms;
    int64_t            next_position_poll_ms;
};

/* ----------------------------------- small helpers ----------------------------------- */

static char* tinympris_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* out = (char*)malloc(len);
    if (out) memcpy(out, s, len);
    return out;
}

static void tinympris_set_str(char** dst, const char* src) {
    free(*dst);
    *dst = tinympris_strdup(src);
}

static int64_t tinympris_monotonic_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* Percent-decodes "%XX" escapes from src into dst (dst must have room for strlen(src)+1). */
static void tinympris_percent_decode(const char* src, char* dst) {
    size_t o = 0;
    for (size_t i = 0; src[i]; i++) {
        if (src[i] == '%' && isxdigit((unsigned char)src[i + 1]) && isxdigit((unsigned char)src[i + 2])) {
            char hex[3] = { src[i + 1], src[i + 2], 0 };
            dst[o++] = (char)strtol(hex, NULL, 16);
            i += 2;
        } else {
            dst[o++] = src[i];
        }
    }
    dst[o] = '\0';
}

/* mpris:artUrl fixup: file:// urls are frequently percent-encoded by players, decode them
 * for direct use. http(s):// and data:image/ urls are left untouched. */
static char* tinympris_correct_art_url(const char* url) {
    static const char file_prefix[] = "file://";
    if (strncmp(url, file_prefix, sizeof(file_prefix) - 1) == 0) {
        char* decoded = (char*)malloc(strlen(url) + 1);
        if (!decoded) return NULL;
        tinympris_percent_decode(url, decoded);
        return decoded;
    }
    return tinympris_strdup(url);
}

/* Derives a friendly identity from a well known name, e.g.
 * "org.mpris.MediaPlayer2.vlc" -> "Vlc" */
static char* tinympris_format_identity(const char* well_known_name) {
    static const char prefix[] = TINYMPRIS_NAME_PREFIX ".";
    const char* name = well_known_name;
    if (strncmp(well_known_name, prefix, sizeof(prefix) - 1) == 0) {
        name = well_known_name + sizeof(prefix) - 1;
    }
    char* out = tinympris_strdup(name);
    if (out && out[0]) out[0] = (char)toupper((unsigned char)out[0]);
    return out;
}

/* ----------------------------------- player list management ----------------------------------- */

static void tinympris_free_metadata(tinympris_metadata* md) {
    free(md->track_id);
    free(md->art_url);
    free(md->album);
    free(md->title);
    free(md->url);
    for (size_t i = 0; i < md->artist_count; i++) free(md->artists[i]);
    free(md->artists);
    memset(md, 0, sizeof(*md));
}

static void tinympris_free_player(tinympris_player* p) {
    if (!p) return;
    free(p->bus_name);
    free(p->well_known_name);
    free(p->identity);
    tinympris_free_metadata(&p->metadata);
    free(p);
}

static long tinympris_find_player_index(const tinympris_ctx* ctx, const char* name) {
    if (!name) return -1;
    for (size_t i = 0; i < ctx->count; i++) {
        tinympris_player* p = ctx->players[i];
        if ((p->bus_name && strcmp(p->bus_name, name) == 0) ||
            (p->well_known_name && strcmp(p->well_known_name, name) == 0)) {
            return (long)i;
        }
    }
    return -1;
}

static tinympris_player* tinympris_add_player(tinympris_ctx* ctx, const char* unique_name, const char* well_known_name) {
    if (ctx->count == ctx->capacity) {
        size_t new_cap = ctx->capacity ? ctx->capacity * 2 : 4;
        tinympris_player** new_arr = (tinympris_player**)realloc(ctx->players, new_cap * sizeof(*new_arr));
        if (!new_arr) return NULL;
        ctx->players = new_arr;
        ctx->capacity = new_cap;
    }

    tinympris_player* p = (tinympris_player*)calloc(1, sizeof(tinympris_player));
    if (!p) return NULL;
    p->bus_name = tinympris_strdup(unique_name);
    p->well_known_name = tinympris_strdup(well_known_name);
    p->identity = tinympris_format_identity(well_known_name);
    p->status = TINYMPRIS_STATUS_UNKNOWN;
    p->loop_status = TINYMPRIS_LOOP_UNKNOWN;

    ctx->players[ctx->count++] = p;
    return p;
}

static void tinympris_remove_player_at(tinympris_ctx* ctx, size_t index) {
    tinympris_free_player(ctx->players[index]);
    for (size_t i = index; i + 1 < ctx->count; i++) {
        ctx->players[i] = ctx->players[i + 1];
    }
    ctx->count--;
}

size_t tinympris_get_player_count(const tinympris_ctx* ctx) {
    return ctx ? ctx->count : 0;
}

const tinympris_player* tinympris_get_player_at(const tinympris_ctx* ctx, size_t index) {
    if (!ctx || index >= ctx->count) return NULL;
    return ctx->players[index];
}

const tinympris_player* tinympris_get_player_by_name(const tinympris_ctx* ctx, const char* name) {
    if (!ctx) return NULL;
    long idx = tinympris_find_player_index(ctx, name);
    return idx >= 0 ? ctx->players[idx] : NULL;
}

void tinympris_set_position_poll_interval_ms(tinympris_ctx* ctx, int interval_ms) {
    if (!ctx) return;
    ctx->position_poll_interval_ms = interval_ms;
}

/* ----------------------------------- metadata dict parsing ----------------------------------- */

static void tinympris_append_artist(tinympris_metadata* md, const char* artist) {
    char** new_arr = (char**)realloc(md->artists, (md->artist_count + 1) * sizeof(char*));
    if (!new_arr) return;
    md->artists = new_arr;
    md->artists[md->artist_count++] = tinympris_strdup(artist);
}

/* Parses an a{sv} Metadata_Map (the value of the "Metadata" property) into `md`. */
static void tinympris_parse_metadata_dict(DBusMessageIter* iter, tinympris_metadata* md) {
    int current_type;
    const char* property_name = NULL;
    DBusMessageIter sub, subsub;

    while ((current_type = dbus_message_iter_get_arg_type(iter)) != DBUS_TYPE_INVALID) {
        switch (current_type) {
        case DBUS_TYPE_DICT_ENTRY:
            dbus_message_iter_recurse(iter, &sub);
            tinympris_parse_metadata_dict(&sub, md);
            break;
        case DBUS_TYPE_STRING:
            dbus_message_iter_get_basic(iter, &property_name);
            break;
        case DBUS_TYPE_VARIANT: {
            if (!property_name) break;
            dbus_message_iter_recurse(iter, &sub);
            int vtype = dbus_message_iter_get_arg_type(&sub);

            if (strcmp(property_name, "xesam:title") == 0 && vtype == DBUS_TYPE_STRING) {
                char* v; dbus_message_iter_get_basic(&sub, &v);
                tinympris_set_str(&md->title, v);
            } else if (strcmp(property_name, "xesam:album") == 0 && vtype == DBUS_TYPE_STRING) {
                char* v; dbus_message_iter_get_basic(&sub, &v);
                tinympris_set_str(&md->album, v);
            } else if (strcmp(property_name, "xesam:url") == 0 && vtype == DBUS_TYPE_STRING) {
                char* v; dbus_message_iter_get_basic(&sub, &v);
                tinympris_set_str(&md->url, v);
            } else if (strcmp(property_name, "mpris:trackid") == 0 &&
                       (vtype == DBUS_TYPE_OBJECT_PATH || vtype == DBUS_TYPE_STRING)) {
                char* v; dbus_message_iter_get_basic(&sub, &v);
                tinympris_set_str(&md->track_id, v);
            } else if (strcmp(property_name, "mpris:length") == 0 &&
                       (vtype == DBUS_TYPE_INT64 || vtype == DBUS_TYPE_UINT64)) {
                int64_t v;
                if (vtype == DBUS_TYPE_INT64) {
                    dbus_message_iter_get_basic(&sub, &v);
                } else {
                    uint64_t uv; dbus_message_iter_get_basic(&sub, &uv); v = (int64_t)uv;
                }
                md->length_us = v;
            } else if (strcmp(property_name, "mpris:artUrl") == 0 && vtype == DBUS_TYPE_STRING) {
                char* v; dbus_message_iter_get_basic(&sub, &v);
                free(md->art_url);
                md->art_url = tinympris_correct_art_url(v);
            } else if (strcmp(property_name, "xesam:trackNumber") == 0 && vtype == DBUS_TYPE_INT32) {
                int32_t v; dbus_message_iter_get_basic(&sub, &v);
                md->track_number = (int)v;
            } else if (strcmp(property_name, "xesam:discNumber") == 0 && vtype == DBUS_TYPE_INT32) {
                int32_t v; dbus_message_iter_get_basic(&sub, &v);
                md->disc_number = (int)v;
            } else if (strcmp(property_name, "xesam:artist") == 0) {
                for (size_t i = 0; i < md->artist_count; i++) free(md->artists[i]);
                free(md->artists);
                md->artists = NULL;
                md->artist_count = 0;
                if (vtype == DBUS_TYPE_ARRAY) {
                    dbus_message_iter_recurse(&sub, &subsub);
                    while (dbus_message_iter_get_arg_type(&subsub) == DBUS_TYPE_STRING) {
                        char* v; dbus_message_iter_get_basic(&subsub, &v);
                        tinympris_append_artist(md, v);
                        dbus_message_iter_next(&subsub);
                    }
                } else if (vtype == DBUS_TYPE_STRING) {
                    char* v; dbus_message_iter_get_basic(&sub, &v);
                    tinympris_append_artist(md, v);
                }
            }
            /* unrecognized properties (e.g. non-standard vlc:* keys) are silently ignored */
        } break;
        default:
            break;
        }
        dbus_message_iter_next(iter);
    }
}

/* ----------------------------------- player property parsing ----------------------------------- */

/* Parses an a{sv} dict of org.mpris.MediaPlayer2.Player properties (as delivered by GetAll or
 * a PropertiesChanged signal) into `p`. Sets *changed to true if anything was updated. */
static void tinympris_parse_player_properties(DBusMessageIter* iter, tinympris_player* p, bool* changed) {
    int current_type;
    const char* property_name = NULL;
    DBusMessageIter sub, subsub;

    while ((current_type = dbus_message_iter_get_arg_type(iter)) != DBUS_TYPE_INVALID) {
        switch (current_type) {
        case DBUS_TYPE_DICT_ENTRY:
            dbus_message_iter_recurse(iter, &sub);
            tinympris_parse_player_properties(&sub, p, changed);
            break;
        case DBUS_TYPE_STRING:
            dbus_message_iter_get_basic(iter, &property_name);
            break;
        case DBUS_TYPE_VARIANT: {
            if (!property_name) break;
            dbus_message_iter_recurse(iter, &sub);
            int vtype = dbus_message_iter_get_arg_type(&sub);

            if (strcmp(property_name, "PlaybackStatus") == 0 && vtype == DBUS_TYPE_STRING) {
                char* v; dbus_message_iter_get_basic(&sub, &v);
                if (strcmp(v, "Playing") == 0) p->status = TINYMPRIS_STATUS_PLAYING;
                else if (strcmp(v, "Paused") == 0) p->status = TINYMPRIS_STATUS_PAUSED;
                else if (strcmp(v, "Stopped") == 0) p->status = TINYMPRIS_STATUS_STOPPED;
                else p->status = TINYMPRIS_STATUS_UNKNOWN;
                *changed = true;
            } else if (strcmp(property_name, "LoopStatus") == 0 && vtype == DBUS_TYPE_STRING) {
                char* v; dbus_message_iter_get_basic(&sub, &v);
                if (strcmp(v, "None") == 0) p->loop_status = TINYMPRIS_LOOP_NONE;
                else if (strcmp(v, "Track") == 0) p->loop_status = TINYMPRIS_LOOP_TRACK;
                else if (strcmp(v, "Playlist") == 0) p->loop_status = TINYMPRIS_LOOP_PLAYLIST;
                else p->loop_status = TINYMPRIS_LOOP_UNKNOWN;
                *changed = true;
            } else if (strcmp(property_name, "Shuffle") == 0 && vtype == DBUS_TYPE_BOOLEAN) {
                dbus_bool_t v; dbus_message_iter_get_basic(&sub, &v);
                p->shuffle = v ? true : false;
                *changed = true;
            } else if (strcmp(property_name, "Rate") == 0 && vtype == DBUS_TYPE_DOUBLE) {
                double v; dbus_message_iter_get_basic(&sub, &v);
                p->rate = v; *changed = true;
            } else if (strcmp(property_name, "Volume") == 0 && vtype == DBUS_TYPE_DOUBLE) {
                double v; dbus_message_iter_get_basic(&sub, &v);
                p->volume = v; *changed = true;
            } else if (strcmp(property_name, "Position") == 0 &&
                       (vtype == DBUS_TYPE_INT64 || vtype == DBUS_TYPE_UINT64)) {
                int64_t v;
                if (vtype == DBUS_TYPE_INT64) {
                    dbus_message_iter_get_basic(&sub, &v);
                } else {
                    uint64_t uv; dbus_message_iter_get_basic(&sub, &uv); v = (int64_t)uv;
                }
                p->position_us = v; *changed = true;
            } else if (strcmp(property_name, "CanGoNext") == 0 && vtype == DBUS_TYPE_BOOLEAN) {
                dbus_bool_t v; dbus_message_iter_get_basic(&sub, &v); p->can_go_next = v ? true : false; *changed = true;
            } else if (strcmp(property_name, "CanGoPrevious") == 0 && vtype == DBUS_TYPE_BOOLEAN) {
                dbus_bool_t v; dbus_message_iter_get_basic(&sub, &v); p->can_go_previous = v ? true : false; *changed = true;
            } else if (strcmp(property_name, "CanPlay") == 0 && vtype == DBUS_TYPE_BOOLEAN) {
                dbus_bool_t v; dbus_message_iter_get_basic(&sub, &v); p->can_play = v ? true : false; *changed = true;
            } else if (strcmp(property_name, "CanPause") == 0 && vtype == DBUS_TYPE_BOOLEAN) {
                dbus_bool_t v; dbus_message_iter_get_basic(&sub, &v); p->can_pause = v ? true : false; *changed = true;
            } else if (strcmp(property_name, "CanSeek") == 0 && vtype == DBUS_TYPE_BOOLEAN) {
                dbus_bool_t v; dbus_message_iter_get_basic(&sub, &v); p->can_seek = v ? true : false; *changed = true;
            } else if (strcmp(property_name, "CanControl") == 0 && vtype == DBUS_TYPE_BOOLEAN) {
                dbus_bool_t v; dbus_message_iter_get_basic(&sub, &v); p->can_control = v ? true : false; *changed = true;
            } else if (strcmp(property_name, "Metadata") == 0 && vtype == DBUS_TYPE_ARRAY) {
                dbus_message_iter_recurse(&sub, &subsub);
                tinympris_parse_metadata_dict(&subsub, &p->metadata);
                *changed = true;
            }
        } break;
        default:
            break;
        }
        dbus_message_iter_next(iter);
    }
}

/* ----------------------------------- dbus name helpers ----------------------------------- */

static char* tinympris_get_name_owner(DBusConnection* conn, const char* name) {
    DBusMessage* msg = dbus_message_new_method_call(
        "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "GetNameOwner");
    if (!msg) return NULL;
    DBusMessageIter args;
    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &name);

    DBusError error;
    dbus_error_init(&error);
    DBusMessage* resp = dbus_connection_send_with_reply_and_block(conn, msg, -1, &error);
    dbus_message_unref(msg);

    char* ret = NULL;
    if (dbus_error_is_set(&error)) {
        dbus_error_free(&error);
    } else if (resp) {
        char* owner;
        if (dbus_message_get_args(resp, &error, DBUS_TYPE_STRING, &owner, DBUS_TYPE_INVALID)) {
            ret = tinympris_strdup(owner);
        } else if (dbus_error_is_set(&error)) {
            dbus_error_free(&error);
        }
        dbus_message_unref(resp);
    }
    return ret;
}

/* Blocking GetAll on org.mpris.MediaPlayer2.Player, used to seed initial player state. */
static void tinympris_query_all_properties(DBusConnection* conn, tinympris_player* p) {
    DBusMessage* msg = dbus_message_new_method_call(
        p->bus_name, TINYMPRIS_PATH, "org.freedesktop.DBus.Properties", "GetAll");
    if (!msg) return;
    const char* iface = TINYMPRIS_PLAYER_IFACE;
    DBusMessageIter args;
    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &iface);

    DBusError error;
    dbus_error_init(&error);
    DBusMessage* resp = dbus_connection_send_with_reply_and_block(conn, msg, -1, &error);
    dbus_message_unref(msg);

    if (dbus_error_is_set(&error)) {
        dbus_error_free(&error);
        return;
    }
    if (!resp) return;

    DBusMessageIter iter, sub;
    dbus_message_iter_init(resp, &iter);
    if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
        bool changed = false;
        dbus_message_iter_recurse(&iter, &sub);
        tinympris_parse_player_properties(&sub, p, &changed);
    }
    dbus_message_unref(resp);
}

/* Bookkeeping for an in-flight async "Position" poll (see tinympris_poll_position). We look
 * the player back up by bus name when the reply arrives rather than keeping a raw pointer,
 * since the player may have been removed while the request was in flight. */
struct tinympris_position_poll_ctx {
    tinympris_ctx* ctx;
    char*          bus_name;
};

static void tinympris_position_poll_ctx_free(void* user_data) {
    struct tinympris_position_poll_ctx* poll_ctx = (struct tinympris_position_poll_ctx*)user_data;
    if (!poll_ctx) return;
    free(poll_ctx->bus_name);
    free(poll_ctx);
}

static void tinympris_position_poll_notify(DBusPendingCall* pending, void* user_data) {
    struct tinympris_position_poll_ctx* poll_ctx = (struct tinympris_position_poll_ctx*)user_data;
    DBusMessage* resp = dbus_pending_call_steal_reply(pending);
    if (!resp) return;

    long idx = tinympris_find_player_index(poll_ctx->ctx, poll_ctx->bus_name);
    if (idx >= 0) {
        DBusMessageIter iter, sub;
        dbus_message_iter_init(resp, &iter);
        if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT) {
            dbus_message_iter_recurse(&iter, &sub);
            int vtype = dbus_message_iter_get_arg_type(&sub);
            int64_t v = 0;
            bool have_value = true;
            if (vtype == DBUS_TYPE_INT64) {
                dbus_message_iter_get_basic(&sub, &v);
            } else if (vtype == DBUS_TYPE_UINT64) {
                uint64_t uv; dbus_message_iter_get_basic(&sub, &uv); v = (int64_t)uv;
            } else {
                have_value = false;
            }

            if (have_value) {
                tinympris_player* p = poll_ctx->ctx->players[idx];
                if (p->position_us != v) {
                    p->position_us = v;
                    if (poll_ctx->ctx->cb) {
                        poll_ctx->ctx->cb(TINYMPRIS_EVENT_PLAYER_UPDATED, p, poll_ctx->ctx->user_data);
                    }
                }
            }
        }
    }
    dbus_message_unref(resp);
}

/* Issues a non-blocking Get("Position") call; the result is applied via
 * tinympris_position_poll_notify once the reply arrives (during a later
 * tinympris_process() call). */
static void tinympris_poll_position(tinympris_ctx* ctx, tinympris_player* p) {
    DBusMessage* msg = dbus_message_new_method_call(
        p->bus_name, TINYMPRIS_PATH, "org.freedesktop.DBus.Properties", "Get");
    if (!msg) return;
    const char* iface = TINYMPRIS_PLAYER_IFACE;
    const char* prop = "Position";
    DBusMessageIter args;
    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &iface);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &prop);

    DBusPendingCall* pending = NULL;
    if (dbus_connection_send_with_reply(ctx->conn, msg, &pending, 2000) && pending) {
        struct tinympris_position_poll_ctx* poll_ctx =
            (struct tinympris_position_poll_ctx*)malloc(sizeof(*poll_ctx));
        if (poll_ctx) {
            poll_ctx->ctx = ctx;
            poll_ctx->bus_name = tinympris_strdup(p->bus_name);
            dbus_pending_call_set_notify(pending, tinympris_position_poll_notify, poll_ctx, tinympris_position_poll_ctx_free);
        }
        dbus_pending_call_unref(pending);
    }
    dbus_message_unref(msg);
}

/* ----------------------------------- signal handling ----------------------------------- */

static DBusHandlerResult tinympris_handle_properties_changed(tinympris_ctx* ctx, DBusMessage* message) {
    const char* sender = dbus_message_get_sender(message);
    long idx = tinympris_find_player_index(ctx, sender);
    if (idx < 0) {
        /* Not a player we're tracking (e.g. a race with NameOwnerChanged) - ignore. */
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    tinympris_player* p = ctx->players[idx];

    DBusMessageIter iter, sub;
    int current_type;
    const char* property_name = NULL;
    bool changed = false;

    dbus_message_iter_init(message, &iter);
    while ((current_type = dbus_message_iter_get_arg_type(&iter)) != DBUS_TYPE_INVALID) {
        if (current_type == DBUS_TYPE_STRING) {
            dbus_message_iter_get_basic(&iter, &property_name);
        } else if (current_type == DBUS_TYPE_ARRAY) {
            if (property_name && strcmp(property_name, TINYMPRIS_PLAYER_IFACE) == 0) {
                int c = dbus_message_iter_get_element_count(&iter);
                if (c > 0) {
                    dbus_message_iter_recurse(&iter, &sub);
                    tinympris_parse_player_properties(&sub, p, &changed);
                }
            }
        }
        dbus_message_iter_next(&iter);
    }

    if (changed && ctx->cb) {
        ctx->cb(TINYMPRIS_EVENT_PLAYER_UPDATED, p, ctx->user_data);
    }
    return DBUS_HANDLER_RESULT_HANDLED;
}

/* The Seeked signal (x: Position) is emitted whenever the position changes in a way that is
 * inconsistent with the Rate property (i.e. an explicit seek), letting us update position_us
 * immediately instead of waiting for the next poll. */
static DBusHandlerResult tinympris_handle_seeked(tinympris_ctx* ctx, DBusMessage* message) {
    const char* sender = dbus_message_get_sender(message);
    long idx = tinympris_find_player_index(ctx, sender);
    if (idx < 0) {
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    int64_t position_us;
    DBusError error;
    dbus_error_init(&error);
    if (!dbus_message_get_args(message, &error, DBUS_TYPE_INT64, &position_us, DBUS_TYPE_INVALID)) {
        if (dbus_error_is_set(&error)) dbus_error_free(&error);
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    tinympris_player* p = ctx->players[idx];
    p->position_us = position_us;
    if (ctx->cb) {
        ctx->cb(TINYMPRIS_EVENT_PLAYER_UPDATED, p, ctx->user_data);
    }
    return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult tinympris_handle_name_owner_changed(tinympris_ctx* ctx, DBusMessage* message) {
    const char* name; const char* old_name; const char* new_name;
    DBusError error;
    dbus_error_init(&error);

    if (!dbus_message_get_args(message, &error,
            DBUS_TYPE_STRING, &name, DBUS_TYPE_STRING, &old_name, DBUS_TYPE_STRING, &new_name,
            DBUS_TYPE_INVALID)) {
        if (dbus_error_is_set(&error)) dbus_error_free(&error);
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    if (strncmp(name, TINYMPRIS_NAME_PREFIX, sizeof(TINYMPRIS_NAME_PREFIX) - 1) != 0) {
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    bool registering = old_name != NULL && strlen(old_name) == 0;
    if (registering) {
        tinympris_player* p = tinympris_add_player(ctx, new_name, name);
        if (p) {
            tinympris_query_all_properties(ctx->conn, p);
            if (ctx->cb) ctx->cb(TINYMPRIS_EVENT_PLAYER_ADDED, p, ctx->user_data);
        }
    } else {
        long idx = tinympris_find_player_index(ctx, old_name);
        if (idx >= 0) {
            tinympris_player* p = ctx->players[idx];
            if (ctx->cb) ctx->cb(TINYMPRIS_EVENT_PLAYER_REMOVED, p, ctx->user_data);
            tinympris_remove_player_at(ctx, (size_t)idx);
        }
    }
    return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult tinympris_filter_fn(DBusConnection* connection, DBusMessage* message, void* user_data) {
    tinympris_ctx* ctx = (tinympris_ctx*)user_data;

    const char* dest = dbus_message_get_destination(message);
    const char* my_name = dbus_bus_get_unique_name(connection);
    if (dest && my_name && strcmp(dest, my_name) != 0) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    const char* path = dbus_message_get_path(message);
    if (!path) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    const char* member = dbus_message_get_member(message);

    if (strcmp(path, TINYMPRIS_PATH) == 0) {
        if (member && strcmp(member, "Seeked") == 0) {
            return tinympris_handle_seeked(ctx, message);
        } else if (member && strcmp(member, "PropertiesChanged") == 0) {
            return tinympris_handle_properties_changed(ctx, message);
        }
    } else if (strcmp(path, "/org/freedesktop/DBus") == 0 && member && strcmp(member, "NameOwnerChanged") == 0) {
        return tinympris_handle_name_owner_changed(ctx, message);
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/* ----------------------------------- lifecycle ----------------------------------- */

static bool tinympris_enumerate_existing_players(tinympris_ctx* ctx) {
    DBusMessage* msg = dbus_message_new_method_call(
        "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "ListNames");
    if (!msg) return false;

    DBusError error;
    dbus_error_init(&error);
    DBusMessage* resp = dbus_connection_send_with_reply_and_block(ctx->conn, msg, -1, &error);
    dbus_message_unref(msg);

    if (dbus_error_is_set(&error)) {
        dbus_error_free(&error);
        return false;
    }
    if (!resp) return false;

    DBusMessageIter iter, sub;
    dbus_message_iter_init(resp, &iter);
    if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
        dbus_message_iter_recurse(&iter, &sub);
        while (dbus_message_iter_get_arg_type(&sub) == DBUS_TYPE_STRING) {
            char* name;
            dbus_message_iter_get_basic(&sub, &name);
            if (strncmp(name, TINYMPRIS_NAME_PREFIX, sizeof(TINYMPRIS_NAME_PREFIX) - 1) == 0) {
                char* unique_name = tinympris_get_name_owner(ctx->conn, name);
                if (unique_name) {
                    tinympris_player* p = tinympris_add_player(ctx, unique_name, name);
                    if (p) {
                        tinympris_query_all_properties(ctx->conn, p);
                        if (ctx->cb) ctx->cb(TINYMPRIS_EVENT_PLAYER_ADDED, p, ctx->user_data);
                    }
                    free(unique_name);
                }
            }
            dbus_message_iter_next(&sub);
        }
    }
    dbus_message_unref(resp);
    return true;
}

tinympris_ctx* tinympris_init(tinympris_callback cb, void* user_data) {
    DBusError error;
    dbus_error_init(&error);
    DBusConnection* conn = dbus_bus_get(DBUS_BUS_SESSION, &error);
    if (dbus_error_is_set(&error)) {
        dbus_error_free(&error);
        return NULL;
    }
    if (!conn) return NULL;

    dbus_connection_set_exit_on_disconnect(conn, FALSE);

    tinympris_ctx* ctx = (tinympris_ctx*)calloc(1, sizeof(tinympris_ctx));
    if (!ctx) return NULL;
    ctx->conn = conn;
    ctx->cb = cb;
    ctx->user_data = user_data;
    ctx->position_poll_interval_ms = 100;
    ctx->next_position_poll_ms = tinympris_monotonic_ms() + ctx->position_poll_interval_ms;

    dbus_bus_add_match(conn,
        "type='signal',interface='org.freedesktop.DBus.Properties',member='PropertiesChanged',path='" TINYMPRIS_PATH "'",
        &error);
    if (dbus_error_is_set(&error)) dbus_error_free(&error);

    dbus_bus_add_match(conn,
        "type='signal',interface='" TINYMPRIS_PLAYER_IFACE "',member='Seeked',path='" TINYMPRIS_PATH "'",
        &error);
    if (dbus_error_is_set(&error)) dbus_error_free(&error);

    dbus_bus_add_match(conn,
        "type='signal',interface='org.freedesktop.DBus',member='NameOwnerChanged',path='/org/freedesktop/DBus'",
        &error);
    if (dbus_error_is_set(&error)) dbus_error_free(&error);

    dbus_connection_add_filter(conn, tinympris_filter_fn, ctx, NULL);
    dbus_connection_flush(conn);

    tinympris_enumerate_existing_players(ctx);

    return ctx;
}

int tinympris_process(tinympris_ctx* ctx, int timeout_ms) {
    if (!ctx || !ctx->conn) return -1;

    DBusDispatchStatus status;
    do {
        if (!dbus_connection_read_write_dispatch(ctx->conn, timeout_ms)) {
            return -1; /* connection closed */
        }
        status = dbus_connection_get_dispatch_status(ctx->conn);
    } while (status == DBUS_DISPATCH_DATA_REMAINS);

    /* Position changes during normal playback do not trigger PropertiesChanged (per spec),
     * so periodically poll it for every player that is currently playing. Seeked signals
     * (handled above, via dispatch) cover the "explicit seek" case immediately. */
    if (ctx->position_poll_interval_ms > 0) {
        int64_t now = tinympris_monotonic_ms();
        if (now >= ctx->next_position_poll_ms) {
            for (size_t i = 0; i < ctx->count; i++) {
                tinympris_player* p = ctx->players[i];
                if (p->status == TINYMPRIS_STATUS_PLAYING) {
                    tinympris_poll_position(ctx, p);
                }
            }
            ctx->next_position_poll_ms = now + ctx->position_poll_interval_ms;
        }
    }

    return 0;
}

void tinympris_shutdown(tinympris_ctx* ctx) {
    if (!ctx) return;
    if (ctx->conn) {
        dbus_connection_remove_filter(ctx->conn, tinympris_filter_fn, ctx);
    }
    /* NOTE: we intentionally do not close/unref ctx->conn - it is a shared connection
     * returned by dbus_bus_get() and libdbus manages its lifetime internally. */
    for (size_t i = 0; i < ctx->count; i++) {
        tinympris_free_player(ctx->players[i]);
    }
    free(ctx->players);
    free(ctx);
}

/* ----------------------------------- control methods ----------------------------------- */

static int tinympris_send_simple_call(tinympris_ctx* ctx, const char* player_name, const char* method) {
    if (!ctx || tinympris_find_player_index(ctx, player_name) < 0) return -1;

    DBusMessage* msg = dbus_message_new_method_call(player_name, TINYMPRIS_PATH, TINYMPRIS_PLAYER_IFACE, method);
    if (!msg) return -1;

    dbus_bool_t ok = dbus_connection_send(ctx->conn, msg, NULL);
    dbus_message_unref(msg);
    if (ok) dbus_connection_flush(ctx->conn);
    return ok ? 0 : -1;
}

int tinympris_play(tinympris_ctx* ctx, const char* player_name) { return tinympris_send_simple_call(ctx, player_name, "Play"); }
int tinympris_pause(tinympris_ctx* ctx, const char* player_name) { return tinympris_send_simple_call(ctx, player_name, "Pause"); }
int tinympris_play_pause(tinympris_ctx* ctx, const char* player_name) { return tinympris_send_simple_call(ctx, player_name, "PlayPause"); }
int tinympris_stop(tinympris_ctx* ctx, const char* player_name) { return tinympris_send_simple_call(ctx, player_name, "Stop"); }
int tinympris_next(tinympris_ctx* ctx, const char* player_name) { return tinympris_send_simple_call(ctx, player_name, "Next"); }
int tinympris_previous(tinympris_ctx* ctx, const char* player_name) { return tinympris_send_simple_call(ctx, player_name, "Previous"); }

int tinympris_seek(tinympris_ctx* ctx, const char* player_name, int64_t offset_us) {
    if (!ctx || tinympris_find_player_index(ctx, player_name) < 0) return -1;

    DBusMessage* msg = dbus_message_new_method_call(player_name, TINYMPRIS_PATH, TINYMPRIS_PLAYER_IFACE, "Seek");
    if (!msg) return -1;
    DBusMessageIter args;
    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_INT64, &offset_us);

    dbus_bool_t ok = dbus_connection_send(ctx->conn, msg, NULL);
    dbus_message_unref(msg);
    if (ok) dbus_connection_flush(ctx->conn);
    return ok ? 0 : -1;
}

int tinympris_set_position(tinympris_ctx* ctx, const char* player_name, const char* track_id, int64_t position_us) {
    if (!ctx || tinympris_find_player_index(ctx, player_name) < 0) return -1;

    DBusMessage* msg = dbus_message_new_method_call(player_name, TINYMPRIS_PATH, TINYMPRIS_PLAYER_IFACE, "SetPosition");
    if (!msg) return -1;
    DBusMessageIter args;
    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_OBJECT_PATH, &track_id);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_INT64, &position_us);

    dbus_bool_t ok = dbus_connection_send(ctx->conn, msg, NULL);
    dbus_message_unref(msg);
    if (ok) dbus_connection_flush(ctx->conn);
    return ok ? 0 : -1;
}

int tinympris_open_uri(tinympris_ctx* ctx, const char* player_name, const char* uri) {
    if (!ctx || tinympris_find_player_index(ctx, player_name) < 0) return -1;

    DBusMessage* msg = dbus_message_new_method_call(player_name, TINYMPRIS_PATH, TINYMPRIS_PLAYER_IFACE, "OpenUri");
    if (!msg) return -1;
    DBusMessageIter args;
    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &uri);

    dbus_bool_t ok = dbus_connection_send(ctx->conn, msg, NULL);
    dbus_message_unref(msg);
    if (ok) dbus_connection_flush(ctx->conn);
    return ok ? 0 : -1;
}

#ifdef __cplusplus
}
#endif

#endif /* TINYMPRIS_IMPLEMENTATION */
