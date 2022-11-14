// use std::fmt;

#[cxx::bridge]
mod ffi {

    #[namespace = "rust_rusty"]
    extern "Rust" {
        fn rust_echo(val: i32) -> i32;
        fn debug(val: i32) -> ();
    }
}


fn debug(val: i32) -> () {
    println!("debug: {}", val);
}


// #[inline(always)]
fn rust_echo(val: i32) -> i32 {
    val+5
}
