cmake_minimum_required( VERSION 2.8.11 )

project( dUpload )

set( CMAKE_AUTOMOC TRUE )
set( CMAKE_INCLUDE_CURRENT_DIR TRUE )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/filters )

find_package(Qt5 COMPONENTS Core Gui Multimedia MultimediaWidgets Network Widgets REQUIRED)
if( WIN32 )
    find_package( Qt5WinExtras REQUIRED )
else( WIN32 )
    find_package( Qt5X11Extras REQUIRED )
endif( WIN32 )

file( GLOB dUpload_INCLUDES "src/*.h" )
file( GLOB dUpload_SOURCES "src/*.cpp" )
file( GLOB dUpload_UI "ui/*.ui" )

include_directories("src/" ${LLVM_INCLUDE_DIRS} "${CLANG_INSTALL_PREFIX}/include/clang")

QT5_ADD_RESOURCES( dUpload_RESOURCES "resources/dUpload.qrc" )
QT5_WRAP_UI( dUpload_UI_SOURCES ${dUpload_UI} )

if( WIN32 )
	add_definitions( -DQT_DLL -DQ_WS_WIN )
    add_definitions( -DUNICODE -D_UNICODE )
	list( APPEND dUpload_RESOURCES "resources/dUpload.rc" )
#	set( EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/bin/" )
else ( WIN32 )
    add_definitions( -DQ_WS_X11 )
endif( WIN32 )

add_executable( dUpload WIN32 MACOSX_BUNDLE ${dUpload_INCLUDES} ${dUpload_SOURCES} ${dUpload_UI_SOURCES} ${dUpload_RESOURCES} )
set_target_properties(dUpload PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries( dUpload Qt5::Core Qt5::Gui Qt5::Multimedia Qt5::MultimediaWidgets Qt5::Network Qt5::Widgets )
if( WIN32 )
    target_link_libraries( dUpload Qt5::WinExtras )
else( WIN32 )
    target_link_libraries( dUpload Qt5::X11Extras X11 )
endif( WIN32 )

if( APPLE )
	#SET( MACOSX_BUNDLE_ICON_FILE Speedcrunch.icns )
	#SET( MACOSX_BUNDLE_SHORT_VERSION_STRING 0.7-beta2 )
	#SET( MACOSX_BUNDLE_VERSION 0.7-beta2 )
	#SET( MACOSX_BUNDLE_LONG_VERSION_STRING Version 0.7-beta2 )
	#SET( CMAKE_OSX_ARCHITECTURES ppc;i386 ) #Comment out if not universal binary

	set_target_properties( dUpload PROPERTIES MACOSX_BUNDLE_INFO_PLIST "resources/Info.plist" )
endif( APPLE )

file( GLOB SRC_FILTERS "src/filters/*.cpp" )
foreach( FILTER_SOURCE ${SRC_FILTERS} )
    get_filename_component( FILTER_NAME "${FILTER_SOURCE}" NAME_WE )
    add_library( ${FILTER_NAME} MODULE "src/filters/${FILTER_NAME}.h" "src/dfilterinterface.h" ${FILTER_SOURCE} )
    target_link_libraries( ${FILTER_NAME} Qt5::Core Qt5::Gui Qt5::Widgets )
endforeach()
