vl_target("Voluma")
    set_kind("binary")
    -- Public packages
    add_packages("fmt", {public = true})
    add_packages("vl_dcmtk", "lodepng", "tinyexr")
    -- Source files
    add_files(
        "Utils/*.cpp",
        "Data/*.cpp",
        "app.cpp"
    )
    -- Include directory
    add_includedirs(".")
