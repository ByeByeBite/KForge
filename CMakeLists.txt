cmake_minimum_required(VERSION 3.28)
project(KForge20 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 导出 compile_commands.json，供 VS Code IntelliSense 读取真实编译参数
# （MSVC 的 C++20 模块导入路径 / STL include 依赖于此，否则会误报大量 module 错误）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/EHsc /utf-8 /O2 /MD /bigobj)
    add_link_options(/SUBSYSTEM:CONSOLE /OPT:REF /OPT:ICF)
endif()

set(MOD ${CMAKE_SOURCE_DIR}/modules/cpp)

# =====================================================================
# C++20 具名模块库（每个 .ixx 一个可链接模块目标）
# =====================================================================
add_library(klogger)
target_compile_features(klogger PUBLIC cxx_std_20)
target_sources(klogger PUBLIC FILE_SET CXX_MODULES FILES ${MOD}/klogger.ixx)
set_target_properties(klogger PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/modules/cpp")

function(kmath_module name muixx)
    add_library(${name})
    target_compile_features(${name} PUBLIC cxx_std_20)
    target_sources(${name} PUBLIC FILE_SET CXX_MODULES FILES ${muixx})
    set_target_properties(${name} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/modules/cpp")
endfunction()

# kbignum 分层模块（kmath/kbignum/）：int/dec/frc/cpx 子模块 + 伞形 + 顶层 kmath
kmath_module(kbigint   ${MOD}/kmath/kbignum/kbigint.ixx)
target_link_libraries(kbigint PUBLIC klogger)

kmath_module(kbigdec   ${MOD}/kmath/kbignum/kbigdec.ixx)
target_link_libraries(kbigdec PUBLIC klogger kbigint)

kmath_module(kbigfrc   ${MOD}/kmath/kbignum/kbigfrc.ixx)
target_link_libraries(kbigfrc PUBLIC klogger kbigint kbigdec)

kmath_module(kbigcpx   ${MOD}/kmath/kbignum/kbigcpx.ixx)
target_link_libraries(kbigcpx PUBLIC klogger kbigint kbigdec kbigfrc)

kmath_module(kbignum   ${MOD}/kmath/kbignum/kbignum.ixx)
target_link_libraries(kbignum PUBLIC kbigint kbigdec kbigfrc kbigcpx)

kmath_module(kmath     ${MOD}/kmath/kmath.ixx)
target_link_libraries(kmath PUBLIC kbignum)

add_library(kson)
target_compile_features(kson PUBLIC cxx_std_20)
target_sources(kson PUBLIC FILE_SET CXX_MODULES FILES ${MOD}/kson.ixx)
target_link_libraries(kson PUBLIC klogger)
set_target_properties(kson PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/modules/cpp")

add_library(ktimer)
target_compile_features(ktimer PUBLIC cxx_std_20)
target_sources(ktimer PUBLIC FILE_SET CXX_MODULES FILES ${MOD}/ktimer.ixx)
target_link_libraries(ktimer PUBLIC klogger)
set_target_properties(ktimer PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/modules/cpp")

add_library(kcli)
target_compile_features(kcli PUBLIC cxx_std_20)
target_sources(kcli PUBLIC FILE_SET CXX_MODULES FILES ${MOD}/kcli.ixx)
target_link_libraries(kcli PUBLIC klogger kson kutil)
set_target_properties(kcli PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/modules/cpp")

add_library(kutil)
target_compile_features(kutil PUBLIC cxx_std_20)
target_sources(kutil PUBLIC FILE_SET CXX_MODULES FILES ${MOD}/kutil.ixx)
target_link_libraries(kutil PUBLIC klogger kson)
set_target_properties(kutil PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/modules/cpp")

# kf 组合模块：仅 re-export，不含自身实现
add_library(kf)
target_compile_features(kf PUBLIC cxx_std_20)
target_sources(kf PUBLIC FILE_SET CXX_MODULES FILES ${MOD}/kf.ixx)
target_link_libraries(kf PUBLIC klogger kson kcli ktimer kutil)
set_target_properties(kf PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/modules/cpp")

# =====================================================================
# 可执行文件：test exe 输出到 test\Release\，相对资源路径 (config/...)
# 由 kson 的根锚定（GetProjectRoot) 解析，与运行目录无关
# =====================================================================
function(kforge_add_exe name)
    cmake_parse_arguments(K "" "" "SRCS;MODS" ${ARGN})
    add_executable(${name} ${K_SRCS})
    target_link_libraries(${name} PRIVATE ${K_MODS})
    set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/test")
endfunction()

# ---- test 可执行文件 ----
file(GLOB TEST_SOURCES ${CMAKE_SOURCE_DIR}/test/*.cpp)
foreach(src ${TEST_SOURCES})
    get_filename_component(tname ${src} NAME_WE)
    set(TEST_MODS kf)
    if(tname STREQUAL "dbgkmath" OR tname STREQUAL "dbgkson" OR tname STREQUAL "dbgkutility")
        list(APPEND TEST_MODS kbignum)
    endif()
    kforge_add_exe(${tname} SRCS ${src} MODS ${TEST_MODS})
endforeach()