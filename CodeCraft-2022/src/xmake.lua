set_languages("cxx11")
add_cxxflags("-Wall -pedantic")

includes("utils")
includes("data_in_out")
includes("optimize")
includes("solve")


target("CodeCraft-2021")
    set_kind("binary")
    set_rules("mode.debug", "mode.release")
    add_files("*.cpp")
    -- set_optimize("fastest")