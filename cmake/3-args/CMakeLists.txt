cmake_minimum_required(VERSION 3.5)
project(test)

function(func named_arg)
    message("ARGV = ${ARGV}")
    message("ARGN = ${ARGN}")
    set(no_values ENABLE1 ENABLE2)
    set(single_values S1 S2)
    cmake_parse_arguments("PPP"
                          "${no_values}"
                          "${single_values}"
                          ""
                          ${ARGN})

    foreach(arg IN LISTS no_values)
        if(PPP_${arg})
            message("PPP_${arg} enabled")
        else()
            message("PPP_${arg} disabled")
        endif()
    endforeach()

    message("PPP_S1 = ${PPP_S1}")
    message("PPP_S2 = ${PPP_S2}")
endfunction()

func(named ENABLE1 S2 222 nnn)
