set(LIBMPDCLIENT_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/include")
set(TAGLIB_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/taglib" CACHE STRING "" FORCE)
set(LIBMPDCLIENT_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/include CACHE STRING "" FORCE)
set(CPPHTTPLIB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib CACHE STRING "" FORCE)

set(MAJOR_VERSION 2)
set(MINOR_VERSION 20)
set(PATCH_VERSION 0)
set(MPD_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}")
set(MPD_DEFAULT_PORT 6600)
set(MPD_DEFAULT_HOST "localhost")
set(MPD_DEFAULT_SOCKET "/var/run/mpd/socket")

find_package(ZLIB REQUIRED)

include(CheckFunctionExists)
include(taglib/ConfigureChecks.cmake)
include(CTest)
include(FeatureSummary)
include(GNUInstallDirs)

check_function_exists(strndup HAVE_STRNDUP)
check_function_exists(getaddrinfo HAVE_GETADDRINFO)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/include)
include_directories(${ZLIB_INCLUDE_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/include/mpd/version.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/include/mpd/version.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libmpd.config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/src/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/taglib/taglib/taglib_config.h.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/taglib/taglib/taglib_config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/taglib/config.h.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/taglib/taglib/config.h)

add_definitions(-DTAGLIB_STATIC)
add_definitions(-DHAVE_CONFIG_H)


if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

endif()

file(GLOB libmpdclient_SRC "${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/src/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/libmpdclient/src/*.c")
include_directories(
  taglib/taglib
  taglib/taglib/toolkit
  taglib/taglib/asf
  taglib/taglib/mpeg
  taglib/taglib/ogg
  taglib/taglib/ogg/flac
  taglib/taglib/flac
  taglib/taglib/mpc
  taglib/taglib/mp4
  taglib/taglib/ogg/vorbis
  taglib/taglib/ogg/speex
  taglib/taglib/ogg/opus
  taglib/taglib/mpeg/id3v2
  taglib/taglib/mpeg/id3v2/frames
  taglib/taglib/mpeg/id3v1
  taglib/taglib/ape
  taglib/taglib/wavpack
  taglib/taglib/trueaudio
  taglib/taglib/riff
  taglib/taglib/riff/aiff
  taglib/taglib/riff/wav
  taglib/taglib/mod
  taglib/taglib/s3m
  taglib/taglib/it
  taglib/taglib/xm
  taglib/3rdparty
)

set(tag_HDRS
  taglib/taglib/taglib_export.h
  taglib/taglib/taglib_config.h
  taglib/taglib/tag.h
  taglib/taglib/fileref.h
  taglib/taglib/audioproperties.h
  taglib/taglib/toolkit/taglib.h
  taglib/taglib/toolkit/tstring.h
  taglib/taglib/toolkit/tlist.h
  taglib/taglib/toolkit/tlist.tcc
  taglib/taglib/toolkit/tstringlist.h
  taglib/taglib/toolkit/tbytevector.h
  taglib/taglib/toolkit/tbytevectorlist.h
  taglib/taglib/toolkit/tbytevectorstream.h
  taglib/taglib/toolkit/tiostream.h
  taglib/taglib/toolkit/tfile.h
  taglib/taglib/toolkit/tfilestream.h
  taglib/taglib/toolkit/tmap.h
  taglib/taglib/toolkit/tmap.tcc
  taglib/taglib/toolkit/tpropertymap.h
  taglib/taglib/toolkit/trefcounter.h
  taglib/taglib/toolkit/tdebuglistener.h
  taglib/taglib/mpeg/mpegfile.h
  taglib/taglib/mpeg/mpegproperties.h
  taglib/taglib/mpeg/mpegheader.h
  taglib/taglib/mpeg/xingheader.h
  taglib/taglib/mpeg/id3v1/id3v1tag.h
  taglib/taglib/mpeg/id3v1/id3v1genres.h
  taglib/taglib/mpeg/id3v2/id3v2.h
  taglib/taglib/mpeg/id3v2/id3v2extendedheader.h
  taglib/taglib/mpeg/id3v2/id3v2frame.h
  taglib/taglib/mpeg/id3v2/id3v2header.h
  taglib/taglib/mpeg/id3v2/id3v2synchdata.h
  taglib/taglib/mpeg/id3v2/id3v2footer.h
  taglib/taglib/mpeg/id3v2/id3v2framefactory.h
  taglib/taglib/mpeg/id3v2/id3v2tag.h
  taglib/taglib/mpeg/id3v2/frames/attachedpictureframe.h
  taglib/taglib/mpeg/id3v2/frames/commentsframe.h
  taglib/taglib/mpeg/id3v2/frames/eventtimingcodesframe.h
  taglib/taglib/mpeg/id3v2/frames/generalencapsulatedobjectframe.h
  taglib/taglib/mpeg/id3v2/frames/ownershipframe.h
  taglib/taglib/mpeg/id3v2/frames/popularimeterframe.h
  taglib/taglib/mpeg/id3v2/frames/privateframe.h
  taglib/taglib/mpeg/id3v2/frames/relativevolumeframe.h
  taglib/taglib/mpeg/id3v2/frames/synchronizedlyricsframe.h
  taglib/taglib/mpeg/id3v2/frames/textidentificationframe.h
  taglib/taglib/mpeg/id3v2/frames/uniquefileidentifierframe.h
  taglib/taglib/mpeg/id3v2/frames/unknownframe.h
  taglib/taglib/mpeg/id3v2/frames/unsynchronizedlyricsframe.h
  taglib/taglib/mpeg/id3v2/frames/urllinkframe.h
  taglib/taglib/mpeg/id3v2/frames/chapterframe.h
  taglib/taglib/mpeg/id3v2/frames/tableofcontentsframe.h
  taglib/taglib/mpeg/id3v2/frames/podcastframe.h
  taglib/taglib/ogg/oggfile.h
  taglib/taglib/ogg/oggpage.h
  taglib/taglib/ogg/oggpageheader.h
  taglib/taglib/ogg/xiphcomment.h
  taglib/taglib/ogg/vorbis/vorbisfile.h
  taglib/taglib/ogg/vorbis/vorbisproperties.h
  taglib/taglib/ogg/flac/oggflacfile.h
  taglib/taglib/ogg/speex/speexfile.h
  taglib/taglib/ogg/speex/speexproperties.h
  taglib/taglib/ogg/opus/opusfile.h
  taglib/taglib/ogg/opus/opusproperties.h
  taglib/taglib/flac/flacfile.h
  taglib/taglib/flac/flacpicture.h
  taglib/taglib/flac/flacproperties.h
  taglib/taglib/flac/flacmetadatablock.h
  taglib/taglib/ape/apefile.h
  taglib/taglib/ape/apeproperties.h
  taglib/taglib/ape/apetag.h
  taglib/taglib/ape/apefooter.h
  taglib/taglib/ape/apeitem.h
  taglib/taglib/mpc/mpcfile.h
  taglib/taglib/mpc/mpcproperties.h
  taglib/taglib/wavpack/wavpackfile.h
  taglib/taglib/wavpack/wavpackproperties.h
  taglib/taglib/trueaudio/trueaudiofile.h
  taglib/taglib/trueaudio/trueaudioproperties.h
  taglib/taglib/riff/rifffile.h
  taglib/taglib/riff/aiff/aifffile.h
  taglib/taglib/riff/aiff/aiffproperties.h
  taglib/taglib/riff/wav/wavfile.h
  taglib/taglib/riff/wav/wavproperties.h
  taglib/taglib/riff/wav/infotag.h
  taglib/taglib/asf/asffile.h
  taglib/taglib/asf/asfproperties.h
  taglib/taglib/asf/asftag.h
  taglib/taglib/asf/asfattribute.h
  taglib/taglib/asf/asfpicture.h
  taglib/taglib/mp4/mp4file.h
  taglib/taglib/mp4/mp4atom.h
  taglib/taglib/mp4/mp4tag.h
  taglib/taglib/mp4/mp4item.h
  taglib/taglib/mp4/mp4properties.h
  taglib/taglib/mp4/mp4coverart.h
  taglib/taglib/mod/modfilebase.h
  taglib/taglib/mod/modfile.h
  taglib/taglib/mod/modtag.h
  taglib/taglib/mod/modproperties.h
  taglib/taglib/it/itfile.h
  taglib/taglib/it/itproperties.h
  taglib/taglib/s3m/s3mfile.h
  taglib/taglib/s3m/s3mproperties.h
  taglib/taglib/xm/xmfile.h
  taglib/taglib/xm/xmproperties.h
)

set(mpeg_SRCS
  taglib/taglib/mpeg/mpegfile.cpp
  taglib/taglib/mpeg/mpegproperties.cpp
  taglib/taglib/mpeg/mpegheader.cpp
  taglib/taglib/mpeg/xingheader.cpp
)

set(id3v1_SRCS
  taglib/taglib/mpeg/id3v1/id3v1tag.cpp
  taglib/taglib/mpeg/id3v1/id3v1genres.cpp
)

set(id3v2_SRCS
  taglib/taglib/mpeg/id3v2/id3v2framefactory.cpp
  taglib/taglib/mpeg/id3v2/id3v2synchdata.cpp
  taglib/taglib/mpeg/id3v2/id3v2tag.cpp
  taglib/taglib/mpeg/id3v2/id3v2header.cpp
  taglib/taglib/mpeg/id3v2/id3v2frame.cpp
  taglib/taglib/mpeg/id3v2/id3v2footer.cpp
  taglib/taglib/mpeg/id3v2/id3v2extendedheader.cpp
  )

set(frames_SRCS
  taglib/taglib/mpeg/id3v2/frames/attachedpictureframe.cpp
  taglib/taglib/mpeg/id3v2/frames/commentsframe.cpp
  taglib/taglib/mpeg/id3v2/frames/eventtimingcodesframe.cpp
  taglib/taglib/mpeg/id3v2/frames/generalencapsulatedobjectframe.cpp
  taglib/taglib/mpeg/id3v2/frames/ownershipframe.cpp
  taglib/taglib/mpeg/id3v2/frames/popularimeterframe.cpp
  taglib/taglib/mpeg/id3v2/frames/privateframe.cpp
  taglib/taglib/mpeg/id3v2/frames/relativevolumeframe.cpp
  taglib/taglib/mpeg/id3v2/frames/synchronizedlyricsframe.cpp
  taglib/taglib/mpeg/id3v2/frames/textidentificationframe.cpp
  taglib/taglib/mpeg/id3v2/frames/uniquefileidentifierframe.cpp
  taglib/taglib/mpeg/id3v2/frames/unknownframe.cpp
  taglib/taglib/mpeg/id3v2/frames/unsynchronizedlyricsframe.cpp
  taglib/taglib/mpeg/id3v2/frames/urllinkframe.cpp
  taglib/taglib/mpeg/id3v2/frames/chapterframe.cpp
  taglib/taglib/mpeg/id3v2/frames/tableofcontentsframe.cpp
  taglib/taglib/mpeg/id3v2/frames/podcastframe.cpp
)

set(ogg_SRCS
  taglib/taglib/ogg/oggfile.cpp
  taglib/taglib/ogg/oggpage.cpp
  taglib/taglib/ogg/oggpageheader.cpp
  taglib/taglib/ogg/xiphcomment.cpp
)

set(vorbis_SRCS
  taglib/taglib/ogg/vorbis/vorbisfile.cpp
  taglib/taglib/ogg/vorbis/vorbisproperties.cpp
)

set(flacs_SRCS
  taglib/taglib/flac/flacfile.cpp
  taglib/taglib/flac/flacpicture.cpp
  taglib/taglib/flac/flacproperties.cpp
  taglib/taglib/flac/flacmetadatablock.cpp
  taglib/taglib/flac/flacunknownmetadatablock.cpp
)

set(oggflacs_SRCS
  taglib/taglib/ogg/flac/oggflacfile.cpp
)

set(mpc_SRCS
  taglib/taglib/mpc/mpcfile.cpp
  taglib/taglib/mpc/mpcproperties.cpp
)

set(mp4_SRCS
  taglib/taglib/mp4/mp4file.cpp
  taglib/taglib/mp4/mp4atom.cpp
  taglib/taglib/mp4/mp4tag.cpp
  taglib/taglib/mp4/mp4item.cpp
  taglib/taglib/mp4/mp4properties.cpp
  taglib/taglib/mp4/mp4coverart.cpp
)

set(ape_SRCS
  taglib/taglib/ape/apetag.cpp
  taglib/taglib/ape/apefooter.cpp
  taglib/taglib/ape/apeitem.cpp
  taglib/taglib/ape/apefile.cpp
  taglib/taglib/ape/apeproperties.cpp
)

set(wavpack_SRCS
  taglib/taglib/wavpack/wavpackfile.cpp
  taglib/taglib/wavpack/wavpackproperties.cpp
)

set(speex_SRCS
  taglib/taglib/ogg/speex/speexfile.cpp
  taglib/taglib/ogg/speex/speexproperties.cpp
)

set(opus_SRCS
  taglib/taglib/ogg/opus/opusfile.cpp
  taglib/taglib/ogg/opus/opusproperties.cpp
)

set(trueaudio_SRCS
  taglib/taglib/trueaudio/trueaudiofile.cpp
  taglib/taglib/trueaudio/trueaudioproperties.cpp
)

set(asf_SRCS
  taglib/taglib/asf/asftag.cpp
  taglib/taglib/asf/asffile.cpp
  taglib/taglib/asf/asfproperties.cpp
  taglib/taglib/asf/asfattribute.cpp
  taglib/taglib/asf/asfpicture.cpp
)

set(riff_SRCS
  taglib/taglib/riff/rifffile.cpp
)

set(aiff_SRCS
  taglib/taglib/riff/aiff/aifffile.cpp
  taglib/taglib/riff/aiff/aiffproperties.cpp
)

set(wav_SRCS
  taglib/taglib/riff/wav/wavfile.cpp
  taglib/taglib/riff/wav/wavproperties.cpp
  taglib/taglib/riff/wav/infotag.cpp
)

set(mod_SRCS
  taglib/taglib/mod/modfilebase.cpp
  taglib/taglib/mod/modfile.cpp
  taglib/taglib/mod/modtag.cpp
  taglib/taglib/mod/modproperties.cpp
)

set(s3m_SRCS
  taglib/taglib/s3m/s3mfile.cpp
  taglib/taglib/s3m/s3mproperties.cpp
)

set(it_SRCS
  taglib/taglib/it/itfile.cpp
  taglib/taglib/it/itproperties.cpp
)

set(xm_SRCS
  taglib/taglib/xm/xmfile.cpp
  taglib/taglib/xm/xmproperties.cpp
)

set(toolkit_SRCS
  taglib/taglib/toolkit/tstring.cpp
  taglib/taglib/toolkit/tstringlist.cpp
  taglib/taglib/toolkit/tbytevector.cpp
  taglib/taglib/toolkit/tbytevectorlist.cpp
  taglib/taglib/toolkit/tbytevectorstream.cpp
  taglib/taglib/toolkit/tiostream.cpp
  taglib/taglib/toolkit/tfile.cpp
  taglib/taglib/toolkit/tfilestream.cpp
  taglib/taglib/toolkit/tdebug.cpp
  taglib/taglib/toolkit/tpropertymap.cpp
  taglib/taglib/toolkit/trefcounter.cpp
  taglib/taglib/toolkit/tdebuglistener.cpp
  taglib/taglib/toolkit/tzlib.cpp
)

set(tag_LIB_SRCS
  ${mpeg_SRCS} ${id3v1_SRCS} ${id3v2_SRCS} ${frames_SRCS} ${ogg_SRCS}
  ${vorbis_SRCS} ${oggflacs_SRCS} ${mpc_SRCS} ${ape_SRCS} ${toolkit_SRCS} ${flacs_SRCS}
  ${wavpack_SRCS} ${speex_SRCS} ${trueaudio_SRCS} ${riff_SRCS} ${aiff_SRCS} ${wav_SRCS}
  ${asf_SRCS} ${mp4_SRCS} ${mod_SRCS} ${s3m_SRCS} ${it_SRCS} ${xm_SRCS} ${opus_SRCS}
  ${zlib_SRCS}
  taglib/taglib/tag.cpp
  taglib/taglib/tagunion.cpp
  taglib/taglib/fileref.cpp
  taglib/taglib/audioproperties.cpp
  taglib/taglib/tagutils.cpp
)

add_library(tag STATIC ${tag_LIB_SRCS} ${tag_HDRS})
add_library(mpdclient STATIC ${libmpdclient_SRC})

target_include_directories(tag PUBLIC ${ZLIB_INCLUDE_DIR})
target_compile_definitions(tag PUBLIC TAGLIB_STATIC)
set_property(TARGET tag PROPERTY POSITION_INDEPENDENT_CODE 1)
set_property(TARGET tag PROPERTY CXX_STANDARD 98)
set_property(TARGET mpdclient PROPERTY POSITION_INDEPENDENT_CODE 1)

set_target_properties(mpdclient PROPERTIES LINKER_LANGUAGE C)
set_target_properties(tag PROPERTIES LINKER_LANGUAGE CXX)

if (APPLE)
target_compile_options(tag
  PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-shorten-64-to-32>
)
endif()

file(GLOB_RECURSE HEADERS "./taglib/taglib/*h" "./taglib/taglib/*.tcc")
foreach(CurrentHeader ${HEADERS})
    get_filename_component(FILE ${CurrentHeader} NAME)
    file(COPY "${CurrentHeader}" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/taglib/taglib/")
endforeach()

include(ConfigureChecks.cmake)

if (HAVE_GCC_ATOMIC)
    target_compile_definitions(tag PUBLIC HAVE_GCC_ATOMIC)
endif()
