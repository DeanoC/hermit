cmake_minimum_required(VERSION 3.12.4)
get_directory_property(hasParent PARENT_DIRECTORY)
if(NOT hasParent)
	option(unittests "unittests" OFF)
	get_filename_component(_PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
	set_property(GLOBAL PROPERTY GLOBAL_FETCHDEPS_BASE ${CMAKE_CURRENT_SOURCE_DIR}/al2o3 )
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
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

set(ComputeCpp_DIR "C:/Program Files/codeplay/computecpp")
set(CMAKE_CUDA_COMPILER "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v10.2/bin/nvcc.exe")
# "spir64"
set(COMPUTECPP_BITCODE "ptx64" CACHE STRING "Bitcode type to use as SYCL target in compute++")

project(${ProjectName} C CXX CUDA)
find_package(ComputeCpp REQUIRED)

if ($ENV{CLION_IDE})
	include_directories("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v10.2\\include")
endif ()

set(Src
		main.cpp
		visualdebugtests.cpp
		synthwaveviztests.c
		meshmodrendertests.cpp
		meshmodrendertests.hpp
		alife/accel_cuda.cu
		alife/accel_cuda.hpp
		alife/accel_sycl.cpp
		alife/accel_sycl.hpp
		alife/world2d.cpp
		alife/world2d.hpp
		alife/alifetests.cpp
		alife/alifetests.hpp
		)
if(APPLE)
	list( APPEND Src Info.plist.in entitlements.plist)
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
		render_basicforward
		render_meshmod
		render_meshmodio
		render_meshmodshapes
		render_meshmodrender
		input_basic_interface
		input_basic_impl_gainput
		gfx_imgui
		gfx_theforge
		tiny_imageformat
		)
ADD_GUI_APP( ${ProjectName} "${Src}" "${Deps}")
add_sycl_to_target(TARGET ${ProjectName} SOURCES alife/accel_sycl.cpp)