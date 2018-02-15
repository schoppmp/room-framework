extern crate oblivc;

use std::env;
use std::path::PathBuf;

fn main() {
    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());

    // Compile `millionaire.oc` using oblivcc
    oblivc::compiler()
        .file("src/bin/benchmark_fss.oc")
        .include("src")
        .include(env::var("DEP_ACK_INCLUDE").unwrap())
        .define("_Float128", "double")
        .compile("benchmark_fss");

    // Generate Rust bindings for the Obliv-C function and struct in `millionaire.h`, then
    // write them to `OUT_DIR/millionaire.rs`.
    oblivc::bindings()
        .header("src/bin/benchmark_fss.h")
        .whitelist_type("BenchmarkFSSArgs")
        .whitelist_function("benchmark_fss")
        .generate().unwrap()
        .write_to_file(out_dir.join("benchmark_fss.rs")).unwrap();

    // Rebuild if either of the files change
    println!("cargo:rerun-if-changed=src/bin/benchmark_fss.h");
    println!("cargo:rerun-if-changed=src/bin/benchmark_fss.oc");
}
