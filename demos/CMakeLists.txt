cmake_minimum_required(VERSION 3.28)
project(demo CXX)

build_application(
    SOURCES
    application.cpp
    
    PACKAGES
    library-template

    LINK_PACKAGES
    library-template
)
