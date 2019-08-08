/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#include <stdint.h>

enum class capability {
    TITLE = 1 << 0,			/* Song title				*/
    ARTIST = 1 << 1,		/* Song artitst				*/
    ALBUM = 1 << 2,			/* Album name				*/
    RELEASE = 1 << 3,		/* Release date 			*/
    COVER = 1 << 4,			/* Cover image link			*/
    LYRICS = 1 << 5,		/* Lyrics text link 		*/
    LENGTH = 1 << 6,		/* Get song length in ms	*/

    /* Control stuff */
    NEXT_SONG = 1 << 7,		/* Skip to next song		*/
    PREV_SONG = 1 << 8,		/* Go to previous song		*/
    PLAY_PAUSE = 1 << 9,	/* Toggle play/pause		*/
    VOLUME_UP = 1 << 10,	/* Increase volume			*/
    VOLUME_DOWN = 1 << 11,	/* Decrease volume			*/
    VOLUME_MUTE = 1 << 12,	/* Toggle mute				*/

    /* Additional info */
    PROGRESS = 1 << 13,		/* Get play progress in ms	*/
    STATUS = 1 << 14		/* Get song playing satus	*/
};

class music_source
{
    uint16_t m_capabilities = 0x0;
    const char* m_name_key; /* Translation key */
    bool m_enabled; 		/* Wether this source can be used */
public:
    music_source(const char* name)
    {
        m_name_key = name;
    }

    virtual ~music_source() {}

    /* util */
    uint16_t get_capabilities() const { return m_capabilities; }

    bool has_capability(capability c) const
    {
        return m_capabilities & ((uint16_t) c);
    }

    /* Abstract stuff */

    virtual void load() = 0;
    /* Perform information query */
    virtual void refresh() = 0;

    /* Execute and return true if successful */
    virtual bool execute_capability(capability c) = 0;
};
