set_project("clox")
set_description("My implemetation of clox. see https://craftinginterpreters.com/")

set_version("0.0.1")
set_xmakever("2.8.5")

set_warnings("all", "error", "pedantic")
set_languages("c11")

add_rules("mode.debug", "mode.release")

target("clox")
    set_kind("binary")
    
    add_files("src/**.c")
    add_headerfiles("src/**.h")