set_languages("cxx11")
add_cxxflags("-Wall -pedantic")

add_links("pthread")

includes("src")

target("CodeCraft-2021")
    set_kind("binary")
    set_rules("mode.debug", "mode.release")
    -- add_files("src/*.cpp")
    -- set_optimize("fastest")
    if(is_mode("debug")) then
        add_defines("TEST")
    else
        set_optimize("fastest")
    end
