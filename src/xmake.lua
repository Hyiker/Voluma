vl_target("Voluma")
    set_kind("binary")
    -- Public packages
    add_packages("fmt", {public = true})
    add_packages("vl_dcmtk")
    -- Source files
    add_files(
        "utils/*.cpp",
        "data/*.cpp",
        "app.cpp"
    )
    -- Include directory
    add_includedirs(".")
