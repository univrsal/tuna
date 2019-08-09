/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#include <stdint.h>
#include <string>
#include <QDate>

enum capability {
    CAP_TITLE = 1 << 0,			/* Song title				*/
    CAP_ARTIST = 1 << 1,		/* Song artitst				*/
    CAP_ALBUM = 1 << 2,			/* Album name				*/
    CAP_RELEASE = 1 << 3,		/* Release date 			*/
    CAP_COVER = 1 << 4,			/* Cover image link			*/
    CAP_LYRICS = 1 << 5,		/* Lyrics text link 		*/
    CAP_LENGTH = 1 << 6,		/* Get song length in ms	*/

    /* Control stuff */
    CAP_NEXT_SONG = 1 << 7,		/* Skip to next song		*/
    CAP_PREV_SONG = 1 << 8,		/* Go to previous song		*/
    CAP_PLAY_PAUSE = 1 << 9,	/* Toggle play/pause		*/
    CAP_VOLUME_UP = 1 << 10,	/* Increase volume			*/
    CAP_VOLUME_DOWN = 1 << 11,	/* Decrease volume			*/
    CAP_VOLUME_MUTE = 1 << 12,	/* Toggle mute				*/

    /* Additional info */
    CAP_PROGRESS = 1 << 13,		/* Get play progress in ms	*/
    CAP_STATUS = 1 << 14		/* Get song playing satus	*/
};

enum date_precision {
    prec_day, prec_month, prec_year, prec_unkown
};

struct song_t {
    uint16_t data;
    std::string title, artists, album;
    uint32_t disc_number, track_number, duration_ms, progress_ms;
    bool is_explicit;
    std::string year, month, day;
    date_precision release_precision;
};

class music_source
{
protected:
    uint16_t m_capabilities = 0x0;
    song_t m_current = {};
public:
    music_source() = default;
    virtual ~music_source() {}

    /* util */
    uint16_t get_capabilities() const { return m_capabilities; }

    bool has_capability(capability c) const
    {
        return m_capabilities & ((uint16_t) c);
    }

    const song_t* song() { return &m_current; }

    /* Abstract stuff */

    /* Save/load config values */
    virtual void load() = 0;
    virtual void save() = 0;

    /* Perform information query */
    virtual void refresh() = 0;

    /* Execute and return true if successful */
    virtual bool execute_capability(capability c) = 0;
};
