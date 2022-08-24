cmake_minimum_required (VERSION 3.16)
project(punch_program_generator)

if (UNIX)
    set(CMAKE_CXX_STANDARD 20)
    set(Qt5_DIR "/lib/x86_64-linux-gnu/cmake/Qt5/")
else()
    set(CMAKE_PREFIX_PATH "C:\\Libraries\\Qt\\5.15.0\\modules\\qtpdf")
    set(Qt5_DIR "C:\\Libraries\\Qt\\5.15.0\\msvc2019_64\\lib\\cmake\\Qt5\\")
    set(Eigen3_DIR "C:\\Libraries\\Eigen3")
    set(QT_PLUGIN_PATH "C:\\Libraries\\Qt\\5.15.0\\msvc2019_64\\plugins")
    set(QT_QPA_PLATFORM_PLUGIN_PATH "C:\\Libraries\\Qt\\5.15.0\\msvc2019_64\\plugins\\platforms")
    set(Qt5PrintSupport_DIR "C:\\ProgramData\\Anaconda3\\Library\\lib\\cmake\\Qt5PrintSupport")
    set(CMAKE_CXX_STANDARD 20)
endif()

if (WIN32)
set(CMAKE_CXX_FLAGS)
elseif(UNIX)

else()
set(CMAKE_CXX_FLAGS -pthread -std=gnu++2a)
endif()

message(STATUS ${Qt5_DIR})

find_package(Qt5 COMPONENTS Quick Core Gui Widgets PrintSupport REQUIRED)
find_package(Qt5Pdf REQUIRED)
find_package(Qt5PdfWidgets REQUIRED)
find_package(Eigen3 REQUIRED NO_MODULE)

set(JSON_INCLUDE_DIRS "C:/Libraries/json/include")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(${PROJECT_NAME} ${EXTRA_ADD_EXECUTABLE_ARGS} ${COMPONENT_WINDOWS} ${WIDGETS} ${PROJECT_UI} ${QT_RESOURCES} "main.cpp" "src/app.cpp" "src/PdfWriter.cpp" "include/PdfWriter.h" "include/UniqueVector.h" "include/MainWindow.h" "src/auto.cpp" "src/manual.cpp")

include_directories(${PROJECT_SOURCE_DIR}/ui ${JSON_INCLUDE_DIRS})


if (MSVC)

add_compile_options(
    #$<$<CONFIG:>:/MT>
    #$<$<CONFIG:Debug>:/MTd>
    #$<$<CONFIG:Release>:/MT>
    "/std:c++17"
    "/wd4068"
)
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Pdf Qt5::PdfWidgets Eigen3::Eigen Qt5::PrintSupport)

else()

target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Pdf Qt5::PdfWidgets Eigen3::Eigen Qt5::PrintSupport)

endif ()
