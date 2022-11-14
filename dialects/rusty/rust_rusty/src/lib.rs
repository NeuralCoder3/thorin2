use autocxx::prelude::*;
use std::pin::Pin;

// include_cpp! {
//     #include "input.h"
//     safety!(unsafe_ffi)
//     generate!("jurassic")
// }

// fn main() {
//     ffi::jurassic();
// }

// #[autocxx::extern_rust::extern_rust_function]
// pub fn new_dinosaur(carnivore: bool) -> Box<Dinosaur> {
//     Box::new(Dinosaur { carnivore })
// }


// #[autocxx::extern_rust::extern_rust_type]
// pub struct Dinosaur {
//     carnivore: bool,
// }

// #[autocxx::extern_rust::extern_rust_function]
// pub fn new_dinosaur(carnivore: bool) -> Box<Dinosaur> {
//     Box::new(Dinosaur { carnivore })
// }

// impl Dinosaur {
//     #[autocxx::extern_rust::extern_rust_function]
//     fn roar(&self) {
//         println!("Roar");
//     }

//     #[autocxx::extern_rust::extern_rust_function]
//     fn eat(self: Pin<&mut Dinosaur>, other_dinosaur: Box<Dinosaur>) {
//         assert!(self.carnivore);
//         other_dinosaur.get_eaten();
//         println!("Nom nom");
//     }

//     fn get_eaten(&self) {
//         println!("Uh-oh");
//     }
// }

// #[autocxx::extern_rust::extern_rust_function]
// pub fn go_extinct() {
//     println!("Boom")
// }


#[autocxx::extern_rust::extern_rust_function]
fn debug(world: i32) -> () {
    println!("Hello, world! {}", world);

}

// #[inline(always)]
#[autocxx::extern_rust::extern_rust_function]
fn rust_echo(val: i32) -> i32 {
    val+7
}
