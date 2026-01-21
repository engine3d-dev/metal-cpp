cmake_minimum_required(VERSION 3.28)
project(demo CXX)

build_application(
    SOURCES
    application.cpp
    
    PACKAGES
    metal-cpp

    LINK_PACKAGES
    metal-cpp
)
