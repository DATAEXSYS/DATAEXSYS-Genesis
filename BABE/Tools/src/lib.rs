pub mod app;
pub mod link;
pub mod trans;
pub mod net;

#[no_mangle]
pub extern "C" fn rust_tool_init() {
    println!("BABE Rust tooling initialized!");
}
