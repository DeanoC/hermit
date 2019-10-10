cmake_minimum_required(VERSION 3.12.4)
get_directory_property(hasParent PARENT_DIRECTORY)
if(NOT hasParent)
	option(unittests "unittests" OFF)
	get_filename_component(_PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
	set_property(GLOBAL PROPERTY GLOBAL_FETCHDEPS_BASE ${_PARENT_DIR}/al2o3 )
	include(FetchContent)
	FetchContent_Declare( al2o3 GIT_REPOSITORY https://github.com/DeanoC/al2o3 GIT_TAG master )
	FetchContent_GetProperties(al2o3)
	if(NOT al2o3_POPULATED)
		FetchContent_Populate(al2o3)
		add_subdirectory(${al2o3_SOURCE_DIR} ${al2o3_BINARY_DIR})
	endif()
	INIT_AL2O3(${CMAKE_CURRENT_SOURCE_DIR})
endif ()

SET_MIN_VERSIONS()
set(ProjectName hermit)
project(${ProjectName})

set(Src
		main.cpp
		visualdebugtests.cpp
		)
if(APPLE)
	list( APPEND Src Info.plist.in)
endif()

set(Deps
    	al2o3_platform
		al2o3_os
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
		gfx_imgui
		gfx_theforge
		)
ADD_GUI_APP( ${ProjectName} "${Src}" "${Deps}")
