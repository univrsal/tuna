// ==UserScript==
// @name         Tuna browser script
// @namespace    univrsal
// @version      1.0.6
// @description  Get song information from web players, based on NowSniper by Kıraç Armağan Önal
// @author       univrsal
// @match        *://open.spotify.com/*
// @match        *://soundcloud.com/*
// @match        *://music.yandex.com/*
// @match        *://music.yandex.ru/*
// @match        *://www.deezer.com/*
// @match        *://play.pretzel.rocks/*
// @grant        unsafeWindow
// @license      GPLv2
// ==/UserScript==

(function() {
    'use strict';
    // Configuration
    var port = 1608;
    var refresh_rate_ms = 500;
    var cooldown_ms = 10000;

    // Tuna isn't running we sleep, because every failed request will log into the console
    // so we don't want to spam it
    var failure_count = 0;
    var cooldown = 0;

    function post(data){
        var url = 'http://localhost:' + port + '/';
        var xhr = new XMLHttpRequest();
        xhr.open('POST', url);

        xhr.setRequestHeader('Accept', 'application/json');
        xhr.setRequestHeader('Content-Type', 'application/json');
        xhr.setRequestHeader('Access-Control-Allow-Headers', '*');
        xhr.setRequestHeader('Access-Control-Allow-Origin', '*');

        xhr.onreadystatechange = function () {
            if (xhr.readyState === 4) {
                if (xhr.status !== 200) {
                    failure_count++;
                }
            }
        };
        xhr.send(JSON.stringify({data,hostname:window.location.hostname,date:Date.now()}));
    }

    // Safely query something, and perform operations on it
    function query(target, fun, alt = null) {
        var element = document.querySelector(target);
        if (element !== null) {
            return fun(element);
        }
        return alt;
    }

    function timestamp_to_ms(ts) {
        var splits = ts.split(':');
        if (splits.length == 2) {
            return splits[0] * 60 * 1000 + splits[1] * 1000;
        } else if (splits.length == 3) {
            return splits[0] * 60 * 60 * 1000 + splits[1] * 60 * 1000 + splits[0] * 1000;
        }
        return 0;
    }

    function StartFunction() {
        setInterval(()=>{
            if (failure_count > 3) {
                console.log('Failed to connect multiple times, waiting a few seconds');
                cooldown = cooldown_ms;
                failure_count = 0;
            }

            if (cooldown > 0) {
                cooldown -= refresh_rate_ms;
                return;
            }

            let hostname = window.location.hostname;

            // TODO: maybe add more?
            if (hostname == 'soundcloud.com') {
                let status = query('.playControl', e => e.classList.contains('playing') ? "playing" : "stopped", 'unknown');
                let cover_url = query('.playbackSoundBadge span.sc-artwork', e => e.style.backgroundImage.slice(5, -2).replace('t50x50','t500x500'));
                let title = query('.playbackSoundBadge__titleLink', e => e.title);
                let artists = [ query('.playbackSoundBadge__lightLink', e => e.title) ];
                let progress = query('.playbackTimeline__timePassed span:nth-child(2)', e => timestamp_to_ms(e.textContent));
                let duration = query('.playbackTimeline__duration span:nth-child(2)', e => timestamp_to_ms(e.textContent));
                let album_url = query('.playbackSoundBadge__titleLink', e => e.href);

                if (title !== null) {
                    post({ cover_url, title, artists, status, progress, duration, album_url });
                }
            } else if (hostname == 'open.spotify.com') {
                let status = query('.player-controls [data-testid="control-button-pause"]', e => !!e ? 'playing' : 'stopped', 'unknown');
                let cover_url = query('[data-testid="CoverSlotExpanded__container"] .cover-art-image', e => e.style.backgroundImage.slice(5, -2));
                let title = query('[data-testid="nowplaying-track-link"]', e => e.textContent);
                let artists = query('span[draggable] a[href*="artist"]', e => Array.from(e));
                let progress = query('.playback-bar .playback-bar__progress-time', e => timestamp_to_ms(e[0].textContent));
                let duration = query('.playback-bar .playback-bar__progress-time', e => timestamp_to_ms(e[1].textContent));
                let album_url = query('[data-testid="nowplaying-track-link"]', e => e.href);

                if (title !== null) {
                    post({ cover_url, title, artists, status, progress, duration, album_url });
                }
            } else if (hostname == 'music.yandex.ru') {
                // Yandex music support by MjKey
                let status = query('.player-controls__btn_play', e => e.classList.contains('player-controls__btn_pause') ? "playing" : "stopped", 'unknown');
                let cover_url = query('.entity-cover__image', e => e.style.backgroundImage.slice(5, -2).replace('50x50','200x200'));
                let title = query('.track__title', e => e.title);
                let artists = [ query('.track__artists', e => e.textContent) ];
                let progress = query('.progress__left', e => timestamp_to_ms(e.textContent));
                let duration = query('.progress__right', e => timestamp_to_ms(e.textContent));
                let album_url = query('.track-cover a', e => e.title);

                if (title !== null) {
                    post({ cover_url, title, artists, status, progress, duration, album_url });
                }
            } else if (hostname == 'www.deezer.com') {
                let status = query('.player-controls', e => {
                    let buttons = e.getElementsByTagName('button');
                    if (buttons && buttons.length > 1) {
                        if (buttons[1].getAttribute('aria-label') === 'Pause') {
                            return "playing";
                        } else {
                            return "stopped";
                        }
                    }
                    return "unknown";
                });

                let cover_url = query('button.queuelist.is-available', e => {
                    let img = e.getElementsByTagName('img');
                    if (img.length > 0) {
                        let src = img[0].src; // https://e-cdns-images.dzcdn.net/images/cover/c4217689cc86e3e6a289162239424dc3/28x28-000000-80-0-0.jpg
                        return src.replace('28x28', '512x512');
                    }
                    return null;
                });

                let title = query('.marquee-content', e => {
                    let links = e.getElementsByClassName('track-link');
                    if (links.length > 0) {
                        return links[0].textContent;
                    }
                    return null;
                });
                let artists = query('.marquee-content', e => {
                    let links = e.getElementsByClassName('track-link');
                    let artists = [];
                    if (links.length > 1) {
                        for (var i = 1; i < links.length; i++) {
                            artists.push(links[i].textContent);
                        }
                        return artists;
                    }
                    return null;
                });

                let duration = query('.slider-counter-max', e => timestamp_to_ms(e.textContent));
                let progress = query('.slider-counter-current', e => timestamp_to_ms(e.textContent));
                if (title !== null) {
                    post({ cover_url, title, artists, status, progress, duration });
                }
            } else if (hostname == "play.pretzel.rocks") {
                // Pretzel.rocks support by Tarulia

                // it seems nothing in the element or page changes
                // there's also no <audio>-element to query status
                // this is a super dirty method by selecting part of the SVG icon path on the Play/Pause Button
                let status = "unknown";

                if (document.querySelector("path[d*='8.064h1.696v12.864h-1.696V9.568zm5.888 0h1.664v12.864h-1.664V9.568z']")) {
                  status = "playing";
                }

                if (document.querySelector("path[d*='8.992l9.568 5.536-9.568 5.536V10.496M11.168 8.64v14.72l12.672-7.328L11.168 8.64z']")) {
                  status = "stopped"
                }

                let cover_url = query('div.rwQJb', e => {
                    let img = e.getElementsByTagName('img');
                    if (img.length > 0) {
                        let src = img[0].src; // https://img.pretzel.rocks/artwork/9Mf8m9/large.jpg
                        return src.replace('medium.jpg', 'large.jpg');
                    }
                    return null;
                });

                let title = query('div.kzpiRD', e => {
                    let elements = e.getElementsByTagName('span');
                    if (elements.length > 0) {
                        return elements[0].textContent;
                    }
                    return null;
                });

                let artists = query('div.kzpiRD', e => {
                    let elements = e.getElementsByTagName('a');
                    if (elements.length > 0) {
                        return elements[0].textContent;
                    }
                    return null;
                });

                let duration = query('div.hcriLb progress', e => e.max * 1000);
                let progress = query('div.hcriLb progress', e => e.value * 1000);

                // is it URL or title? soundcloud/spotify use href, yandax uses title
                let album_url = query('div.kzpiRD', e => {
                    let elements = e.getElementsByTagName('a');
                    if (elements.length > 1) {
                        return elements[1].textContent;
                    }
                    return null;
                });

                if (title !== null) {
                    post({ cover_url, title, artists, status, progress, duration, album_url });
                }
            }
        }, refresh_rate_ms);

    }

    StartFunction();
})();
