// fn main() -> miette::Result<()> {
//     // let _build = cxx_build::bridge("src/lib.rs");
//     let path = std::path::PathBuf::from("src");
//     let mut b = autocxx_build::Builder::new("src/lib.rs", [&path]).build()?;
//     // b.flag_if_supported("-std=c++14").compile("autocxx-demo");
//     b.flag_if_supported("-std=c++14").compile("rust_rusty");

//     println!("cargo:rerun-if-changed=src/lib.rs");
//     Ok(())
// }

fn main() -> miette::Result<()> {
    // let _build = autocxx_build::Builder::new("src/lib.rs", &["src"]).build()?;
    // let path = std::path::PathBuf::from("src");
    // let mut b = autocxx_build::Builder::new("src/lib.rs", &[&path])
    //     .auto_allowlist(true)
    //     .build()?;
    // b.flag_if_supported("-std=c++17")
    //     // .file("src/input.cc")
    //     .compile("rust_rusty");

    let path = std::path::PathBuf::from("src"); // include path
    let mut b = autocxx_build::Builder::new("src/lib.rs", &[&path]).build()?;
        // This assumes all your C++ bindings are in main.rs
    b.flag_if_supported("-std=c++14").compile("rust_rusty");

    println!("cargo:rerun-if-changed=src/lib.rs");
    // println!("cargo:rerun-if-changed=src/input.h");
    Ok(())
}
