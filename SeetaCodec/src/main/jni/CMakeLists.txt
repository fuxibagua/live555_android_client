cmake_minimum_required(VERSION 3.4.1)


file(GLOB native_srcs "${CMAKE_SOURCE_DIR}/*.cpp")

add_library(rtsp_client
            SHARED
            ${native_srcs}
            )

add_library(opencv SHARED IMPORTED)

set_target_properties(opencv
                        PROPERTIES IMPORTED_LOCATION
                        ${CMAKE_SOURCE_DIR}/../../../../third-party/opencv/libs/${ANDROID_ABI}/libopencv_java3.so
)

add_library(live555 STATIC IMPORTED)

set_target_properties(live555
                      PROPERTIES IMPORTED_LOCATION
                      ${CMAKE_SOURCE_DIR}/live555/libs/${ANDROID_ABI}/liblive555.a
)

include_directories(${CMAKE_SOURCE_DIR}/include
                    ${CMAKE_SOURCE_DIR}/../../../../third-party/opencv/jni/include
                    ${CMAKE_SOURCE_DIR}/live555/include/BasicUsageEnvironment/include
                    ${CMAKE_SOURCE_DIR}/live555/include/UsageEnvironment/include
                    ${CMAKE_SOURCE_DIR}/live555/include/groupsock/include
                    ${CMAKE_SOURCE_DIR}/live555/include/liveMedia/include
)

find_library(
            android-lib
            android
            )

find_library(
            openmax-lib
            OpenMAXAL
    )

find_library(
            media-lib
            mediandk
            )

find_library(
            z-lib
            z
)

find_library(
            log-lib
            log
)


target_link_libraries( # Specifies the target library.
                       rtsp_client
                       live555
                       opencv
                       # Links the log library to the target library.
                       ${log-lib}
                       ${android-lib}
                       ${openmax-lib}
                       ${media-lib}
                       ${z-lib}
                       )