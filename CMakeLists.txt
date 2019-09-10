cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

get_directory_property(hasParent PARENT_DIRECTORY)
if(NOT hasParent)
	get_filename_component(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
	set_property(GLOBAL PROPERTY GLOBAL_FETCHDEPS_BASE ${PARENT_DIR}/al2o3 )
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/out_libs)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/out_bin)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/out_bin)
	set(LIB_BASE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/libs/)
	include(FetchContent)
	FetchContent_Declare( al2o3 GIT_REPOSITORY https://github.com/DeanoC/al2o3 GIT_TAG master )
	FetchContent_GetProperties(al2o3)
	if(NOT al2o3_POPULATED)
		FetchContent_Populate(al2o3)
		add_subdirectory(${al2o3_SOURCE_DIR} ${al2o3_BINARY_DIR})
	endif()
endif ()

SET_MIN_VERSIONS()
set(ProjectName hermit)
project(${ProjectName})

set(Src
		main.cpp
		)

set(Deps
    	al2o3_platform
		al2o3_enki
		gfx_imageio
		gfx_image_interface
		gfx_image_impl_basic
		utils_simple_logmanager
		utils_gameappshell_interface
		utils_gameappshell_impl_basic
		render_basics_interface
		render_basics_impl_theforge
		input_basic_interface
		input_basic_impl_gainput
		gfx_imgui_al2o3_theforge_bindings
		gfx_imgui
		utils_nativefiledialogs
		gfx_theforge
		)
ADD_GUI_APP( ${ProjectName} "${Src}" "${Deps}" utils_gameappshell_interface)
