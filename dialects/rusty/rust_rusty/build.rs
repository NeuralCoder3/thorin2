fn main() -> miette::Result<()> {
    // let _build = cxx_build::bridge("src/lib.rs");
    let path = std::path::PathBuf::from("src");
    let mut b = autocxx_build::Builder::new("src/lib.rs", [&path]).build()?;
    // b.flag_if_supported("-std=c++14").compile("autocxx-demo");
    b.flag_if_supported("-std=c++14").compile("rust_rusty");

    println!("cargo:rerun-if-changed=src/lib.rs");
    Ok(())
}
