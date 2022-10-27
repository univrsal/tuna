![header](https://obsproject.com/forum/attachments/screenshot_20201013_230446-jpg.62004/ "tuna running on obs linux")

[![Plugin Build](https://github.com/univrsal/tuna/actions/workflows/main.yml/badge.svg)](https://github.com/univrsal/tuna/actions/workflows/main.yml)

# tuna
Get song info from right within obs.

Issues have moved to [git.vrsal.xyz/alex/tuna/issues](https://git.vrsal.xyz/alex/tuna/issues). You can sign up to submit issues.
    
Currently supports
- Spotify
- MPD
- Any Window title
- [last.fm](https://last.fm) scrobbling
- OBS VLC source
- [YouTube Music](https://github.com/th-ch/youtube-music)
- Most music players through [MPRIS](https://specifications.freedesktop.org/mpris-spec/latest/) and [Windows Media Control](https://learn.microsoft.com/en-us/uwp/api/windows.media.control?view=winrt-19041)
- Through [tampermonkey script](https://github.com/univrsal/tuna/raw/master/deps/tuna_browser.user.js):
    - Soundcloud 
    - Spotify Web Player
    - Deezer
    - Yandex Music
    - Pretzel.rocks

<img src="src/gui/images/tuna.png" alt="hey tuna" width="180px">

### Translators
- [COOLIGUAY](https://github.com/COOLIGUAY) (Spanish) 
- [dEN5-tech](https://github.com/dEN5-tech) (Russian)
- [Cyame](https://github.com/Cyame) (Simplified Chinese) 
- [gabrielpastori1](https://github.com/gabrielpastori1) (Brazilian Portuguese) 
- [orion78fr](https://github.com/orion78fr) (French) 

### Additional credits

- MPRIS and Windows Media Control support taken from [obs_media_info_plugin](https://github.com/rmoalic/obs_media_info_plugin).
- Metadata extraction via [Taglib](https://taglib.org/)
- MPD connection via [libmpdclient](https://musicpd.org/libs/libmpdclient/)
- Webserver implemented through [cpp-httplib](https://github.com/yhirose/cpp-httplib)
- [cURL](https://curl.se) for fetching remote content and API interactions
