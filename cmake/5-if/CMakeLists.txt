cmake_minimum_required(VERSION 3.4)
project(test)

set(var 1)

# var = ${var} = "${var}"

if(var EQUAL 1)
    message("EQUAL")
else()
    message("NOT EQUAL")
endif()

if(${var} EQUAL 1)
    message("EQUAL")
else()
    message("NOT EQUAL")
endif()

if("${var}" EQUAL 1)
    message("EQUAL")
else()
    message("NOT EQUAL")
endif()
