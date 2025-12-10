fn main() {
    cc::Build::new()
        .cpp(true)
        .file("user_space/app.cpp")
        .cpp_link_stdlib(None) 
        .flag("-ffreestanding")
        .flag("-nostdlib")
        .flag("-fno-rtti")
        .flag("-fno-exceptions")
        .flag("-fno-use-cxa-atexit")
        .flag("-fno-threadsafe-statics")
        .compile("user_app");
    println!("cargo:rerun-if-changed=user_space/app.cpp");
}