cmake_minimum_required(VERSION 4.0)
project(test_package CXX)

build_application(
    SOURCES
    application.cpp
    
    PACKAGES
    library-template

    LINK_PACKAGES
    library-template
)
