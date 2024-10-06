vl_target("Voluma")
    set_kind("binary")
    -- Public packages
    add_packages("fmt", {public = true})
    add_packages("vl_dcmtk", "lodepng", 
                 "tinyexr", "glfw", "glm", 
                 "imgui", "slangd")
    -- Source files
    add_files(
        "Core/**.cpp",
        "Utils/**.cpp",
        "Data/**.cpp",
        "Voluma.cpp",

        "Shaders/*.slang"
    )
    -- Include directory
    add_includedirs(".")
