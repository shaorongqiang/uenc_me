cmake_minimum_required(VERSION 3.18)

set(UENC_VERSION "1.6.4")
project (uenc VERSION ${UENC_VERSION})

set(CMAKE_CXX_STANDARD 17)

set(CXX_FLAGS -Wall)
set(C_FLAGS -Wall)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -rdynamic")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -rdynamic")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -rdynamic")
set(CMAKE_C_FLAGS_RELEASE  "${CMAKE_C_FLAGS_RELEASE} -rdynamic")
add_definitions(-Wno-builtin-macro-redefined)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(EXECUTABLE_OUTPUT_PATH bin)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE debug)
endif()
message("-- CMAKE_BUILD_TYPE is: ${CMAKE_BUILD_TYPE}")

# 打印编译时间
set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")
set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CMAKE_COMMAND} -E time")

execute_process(COMMAND sh ${PROJECT_SOURCE_DIR}/make_depend.sh ${PROJECT_SOURCE_DIR}/3rd ${CMAKE_CURRENT_BINARY_DIR}/lib)

if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/proto)
    set(GENERATEDPROTO OFF)
else()
    set(GENERATEDPROTO ON)
endif()
if(GENERATEDPROTO)
    file(GLOB protobuf_protocol_files ${PROJECT_SOURCE_DIR}/proto/*.proto)
    EXECUTE_PROCESS(COMMAND mkdir ${CMAKE_CURRENT_BINARY_DIR}/proto)
    FOREACH(FILE ${protobuf_protocol_files})
        GET_FILENAME_COMPONENT(FILE_WE ${FILE} NAME_WE)
        string(REGEX REPLACE ".+/(.+)\\..*" "\\1" FILE_NAME ${FILE})
        string(REGEX REPLACE "(.+)\\${FILE_NAME}.*" "\\1" FILE_PATH ${FILE})
        EXECUTE_PROCESS(COMMAND ${CMAKE_CURRENT_BINARY_DIR}/lib/protobuf/build/protoc -I${PROJECT_SOURCE_DIR}/proto --cpp_out=${CMAKE_CURRENT_BINARY_DIR}/proto ${FILE})
    ENDFOREACH()
endif()

#重新定义当前目标的源文件的__FILE__宏
function(redefine_file_macro targetname)
    #获取当前目标的所有源文件
    get_target_property(source_files "${targetname}" SOURCES)
    #遍历源文件
    foreach(sourcefile ${source_files})
        #获取当前源文件的编译参数
        get_property(defs SOURCE "${sourcefile}"
            PROPERTY COMPILE_DEFINITIONS)
        #获取当前文件的绝对路径
        get_filename_component(filepath "${sourcefile}" ABSOLUTE)
        #将绝对路径中的项目路径替换成空,得到源文件相对于项目路径的相对路径
        string(REPLACE ${PROJECT_SOURCE_DIR}/ "" relpath ${filepath})
        string(REPLACE ".cpp" "" relpath ${relpath})
        string(REPLACE ".c" "" relpath ${relpath})
        #将我们要加的编译参数(__FILE__定义)添加到原来的编译参数里面
        list(APPEND defs "__FILE__=\"${relpath}\"")
        #重新设置源文件的编译参数
        set_property(
            SOURCE "${sourcefile}"
            PROPERTY COMPILE_DEFINITIONS ${defs}
            )
    endforeach()
endfunction()

include_directories( ${PROJECT_SOURCE_DIR}/ )
include_directories( ${PROJECT_SOURCE_DIR}/include)
include_directories( ${CMAKE_CURRENT_BINARY_DIR}/ )
include_directories( ${CMAKE_CURRENT_BINARY_DIR}/lib )
include_directories( ${CMAKE_CURRENT_BINARY_DIR}/lib/protobuf/src )
include_directories( ${CMAKE_CURRENT_BINARY_DIR}/lib/boost/boost )
include_directories( ${CMAKE_CURRENT_BINARY_DIR}/lib/nlohmann_json/include )
include_directories( ${CMAKE_CURRENT_BINARY_DIR}/lib/libevent/include )
include_directories( ${CMAKE_CURRENT_BINARY_DIR}/lib/libevent/build/include )
include_directories( ${CMAKE_CURRENT_BINARY_DIR}/lib/rocksdb/include )
include_directories( ${CMAKE_CURRENT_BINARY_DIR}/lib/libbase58 )
include_directories( ${CMAKE_CURRENT_BINARY_DIR}/lib/spdlog/include )

file(GLOB PROTO_SRCS ${CMAKE_CURRENT_BINARY_DIR}/proto/*.pb.cc)
aux_source_directory(common SOURCE_FILES )
aux_source_directory(utils SOURCE_FILES )
aux_source_directory(socket SOURCE_FILES )
aux_source_directory(account SOURCE_FILES )
aux_source_directory(db SOURCE_FILES )
aux_source_directory(node SOURCE_FILES )
aux_source_directory(http SOURCE_FILES )
aux_source_directory(ca SOURCE_FILES )

add_executable(${PROJECT_NAME} main.cpp ${SOURCE_FILES} ${PROTO_SRCS})
target_link_libraries(${PROJECT_NAME} pthread )
target_link_libraries(${PROJECT_NAME} -lz -lstdc++fs -static-libgcc -static-libstdc++)

add_library(protobuf STATIC IMPORTED)
set_property(TARGET protobuf PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/lib/protobuf/build/libprotobuf.a)
target_link_libraries(${PROJECT_NAME} protobuf )

add_library(cryptopp STATIC IMPORTED)
set_property(TARGET cryptopp PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/lib/cryptopp/libcryptopp.a)
target_link_libraries(${PROJECT_NAME} cryptopp )

add_library(event STATIC IMPORTED)
set_property(TARGET event PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/lib/libevent/build/lib/libevent.a)
target_link_libraries(${PROJECT_NAME} event )

add_library(base58 STATIC IMPORTED)
set_property(TARGET base58 PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/lib/libbase58/.libs/libbase58.a)
target_link_libraries(${PROJECT_NAME} base58 )

add_library(rocksdb STATIC IMPORTED)
set_property(TARGET rocksdb PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/lib/rocksdb/build/librocksdb.a)
target_link_libraries(${PROJECT_NAME} rocksdb )

add_library(spdlog STATIC IMPORTED)
set_property(TARGET spdlog PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/lib/spdlog/build/libspdlog.a)
target_link_libraries(${PROJECT_NAME} spdlog )

find_package(GTest)
if(GTEST_FOUND)
    file(GLOB_RECURSE TEST_SOURCE test/*.cpp)
    include_directories(${GTEST_INCLUDE_DIRS})
    SET(PROJECT_TEST test)
    add_executable(${PROJECT_TEST} EXCLUDE_FROM_ALL ${SOURCES} ${TEST_SOURCE} ${SOURCE_FILES} ${PROTO_SRCS})
    target_link_libraries(${PROJECT_TEST} pthread )
    target_link_libraries(${PROJECT_TEST} -lz -lstdc++fs -static-libgcc -static-libstdc++)
    target_link_libraries(${PROJECT_TEST} ${GTEST_BOTH_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
    target_link_libraries(${PROJECT_TEST} protobuf )
    target_link_libraries(${PROJECT_TEST} cryptopp )
    target_link_libraries(${PROJECT_TEST} event )
    target_link_libraries(${PROJECT_TEST} base58 )
    target_link_libraries(${PROJECT_TEST} rocksdb )
    target_link_libraries(${PROJECT_TEST} spdlog )
endif(GTEST_FOUND)

#set(PRIMARYCHAIN ON)

if(PRIMARYCHAIN)
    SET(CHAIN_VERSION primarynet)
    ADD_DEFINITIONS(-DPRIMARYCHAIN)
elseif(TESTCHAIN)
    SET(CHAIN_VERSION testnet)
    ADD_DEFINITIONS(-DTESTCHAIN)
else()
    SET(CHAIN_VERSION devnet)
    ADD_DEFINITIONS(-DDEVCHAIN)
endif()
ADD_DEFINITIONS(-D_VERSION=\"${UENC_VERSION}\")
find_package(Git QUIET)
if(GIT_FOUND)
    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND echo filename: ${PROJECT_NAME}_${UENC_VERSION}_`${GIT_EXECUTABLE} symbolic-ref --short -q HEAD`_`${GIT_EXECUTABLE} rev-parse --short HEAD`_${CHAIN_VERSION}_${CMAKE_BUILD_TYPE}
        COMMAND mv ${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}
        ${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}_${UENC_VERSION}_`${GIT_EXECUTABLE} symbolic-ref --short -q HEAD`_`${GIT_EXECUTABLE} rev-parse --short HEAD`_${CHAIN_VERSION}_${CMAKE_BUILD_TYPE})
else()
    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND echo filename: ${PROJECT_NAME}_${UENC_VERSION}_${CHAIN_VERSION}_${CMAKE_BUILD_TYPE}
        COMMAND mv ${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}
        ${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}_${UENC_VERSION}_${CHAIN_VERSION}_${CMAKE_BUILD_TYPE})
endif()
