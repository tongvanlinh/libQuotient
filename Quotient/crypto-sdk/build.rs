use cxx_build::CFG;

fn main() {
    CFG.include_prefix = "matrix-rust-sdk-crypto";
    cxx_build::bridge("src/lib.rs")
        .std("c++20")
        .define("__ANDROID_API__", "21")
        .compile("matrix-rust-sdk-crypto");
}
