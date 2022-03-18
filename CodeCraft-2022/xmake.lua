set_languages("cxx11")
add_cxxflags("-Wall -pedantic")

target("CV")
    set_kind("binary")
    set_rules("mode.debug", "mode.release")
    add_files("src/*.cpp")
    -- set_optimize("fastest")