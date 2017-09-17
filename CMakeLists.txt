cmake_minimum_required(VERSION 3.1.0)

project(WaveBuzzer)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc, uic and rcc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find the QtWidgets library (also finds Core and Gui)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Multimedia REQUIRED)
find_package(Qt5MultimediaWidgets REQUIRED)

#-------------------------------------------------------------------------------
#set up compiler flags and excutable names

# Executables fail to build with Qt 5 in the default configuration without -fPIE. We add that here.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
	#set up compiler flags for Clang
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2") #support C++11 for std::, optimize
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O2") #optimize
elseif (${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
	#check for G++ 5.4+
	execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE G++_VERSION)
	if (G++_VERSION VERSION_LESS 5.4)
			message(SEND_ERROR "You need at least G++ 5.4 to compile ${PROJECT_NAME}!")
	endif()
	#set up compiler flags for GCC
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2") #support C++11 for std::, optimize
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O2") #optimize
elseif (${CMAKE_CXX_COMPILER_ID} MATCHES "MSVC")
	set(CMAKE_DEBUG_POSTFIX "d")
	add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
	add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP") #multi-processor compilation
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP") #multi-processor compilation
endif()

#-------------------------------------------------------------------------------
#define basic sources and headers

set(WAVEBUZZER_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/joystick.h
	${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.h
	${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/videowidget.h
)

set(WAVEBUZZER_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc
    ${CMAKE_CURRENT_SOURCE_DIR}/videowidget.cpp
)

#-------------------------------------------------------------------------------
#define OS specific sources and headers

if(WIN32)
	LIST(APPEND WAVEBUZZER_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/joystick_win.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/resources.rc
	)
elseif(UNIX AND NOT APPLE)
	LIST(APPEND WAVEBUZZER_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/joystick_linux.cpp
	)
endif()

#-------------------------------------------------------------------------------
#define libraries and directories

if(WIN32)
	set(WAVEBUZZER_LIBRARIES
		winmm
	)
elseif(UNIX AND NOT APPLE)
	set(WAVEBUZZER_LIBRARIES
		pthread
	)
endif()

#-------------------------------------------------------------------------------
#set up build directories

set(dir ${CMAKE_CURRENT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)
set(LIBRARY_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)

#-------------------------------------------------------------------------------
#define target

include_directories(${Qt5Widgets_INCLUDES})
add_executable(${PROJECT_NAME} ${WAVEBUZZER_SOURCES} ${WAVEBUZZER_HEADERS})
target_link_libraries(${PROJECT_NAME} ${Qt5Widgets_LIBRARIES} Qt5::Multimedia Qt5::MultimediaWidgets ${WAVEBUZZER_LIBRARIES})

#-------------------------------------------------------------------------------
#special properties for windows builds
if(MSVC)
	#create console application
	#Note that up to CMake 2.8.11.2 this feature is broken: http://public.kitware.com/Bug/view.php?id=12566
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE")
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
endif()
