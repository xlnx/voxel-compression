add_subdirectory(nvdec)
get_directory_property(NVDEC_SRC DIRECTORY nvdec DEFINITION SOURCES)

if (OPENH264_FOUND)
    add_subdirectory(openh264)
    get_directory_property(OPENH264_SRC DIRECTORY openh264 DEFINITION SOURCES)
endif()

set(SOURCES ${NVDEC_SRC} ${OPENH264_SRC})
