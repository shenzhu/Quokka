﻿# CMakeList.txt : CMake project for Quokka, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

# Add source to this project's executable.
add_executable (
	Quokka
	main.cc
	util/StringView.h
	util/StringView.cc
	util/Timer.h
	util/Timer.cc
	util/TimeUtil.h
	util/TimeUtil.cc
	util/ThreadPool.h
	util/ThreadPool.cc
	util/buffer.h
	util/buffer.cc
	future/Try.h
	future/Scheduler.h
	future/Helper.h
	future/Future.h
)
