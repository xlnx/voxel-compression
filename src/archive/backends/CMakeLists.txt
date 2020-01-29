add_subdirectory(nvenc)
get_directory_property(NVENC_SRC DIRECTORY nvenc DEFINITION SOURCES)

if (OPENH264_FOUND)
    add_subdirectory(openh264)
    get_directory_property(OPENH264_SRC DIRECTORY openh264 DEFINITION SOURCES)
endif()

set(SOURCES ${NVENC_SRC} ${OPENH264_SRC})
