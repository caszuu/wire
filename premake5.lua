workspace("wire")
    configurations { "debug", "release" }
    
    filter("configurations:debug")
        symbols "On"
        defines { "DEBUG" }
    
    filter("configurations:release")
        defines { "NDEBUG" }
        buildoptions { "-Wall" }
        optimize "Full"
    
    filter()

    project("wire-server")
        targetname  ("%{prj.name}")
        targetdir   ("bin/")

        kind        ("ConsoleApp")
        language    ("C++")

        cppdialect  ("C++20")
        cdialect    ("C99")

        files {
            -- wire-server
            "src/**",
            
            -- wren vm
            "ext/wren/src/**.h",
            "ext/wren/src/**.c",
        }
        
        includedirs {
            "ext/wren/src/**"
        }

        