extern crate oblivc;

use std::env;
use std::path::PathBuf;
use std::process::Command;

fn main() {
    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());
    // update submodules, but ignore if that fails
    let _status = Command::new("git")
        .args(&["submodule", "update", "--init", "--recursive"])
        .status();

    // Compile using oblivcc
    oblivc::compiler()
        .file("src/oblivc/benchmark_fss.oc")
        .file("src/oblivc/benchmark_scs.oc")
        .file("src/oblivc/benchmark_common.c")
        .include(env::var("DEP_ACK_INCLUDE").unwrap())
        .define("_Float128", "double") // workaround for https://github.com/samee/obliv-c/issues/48
        .flag_if_supported("-Wno-unused-parameter")
        .flag_if_supported("-Wno-unused-function")
        .flag_if_supported("-Wno-unused-const-variable") // silence compiler warnings about Obliv-C headers
        .compile("benchmarks");

    // Generate Rust bindings for the Obliv-C function
    oblivc::bindings()
        .header("src/oblivc/benchmark_fss.h")
        .header("src/oblivc/benchmark_scs.h")
        .whitelist_type("BenchmarkFSSArgs")
        .whitelist_function("benchmark_fss")
        .whitelist_type("BenchmarkSCSArgs")
        .whitelist_function("benchmark_scs")
        .generate().unwrap()
        .write_to_file(out_dir.join("benchmarks.rs")).unwrap();

    // Rebuild if either of the files change
    println!("cargo:rerun-if-changed=src/oblivc/benchmark_fss.h");
    println!("cargo:rerun-if-changed=src/oblivc/benchmark_fss.oc");
    println!("cargo:rerun-if-changed=src/oblivc/benchmark_scs.h");
    println!("cargo:rerun-if-changed=src/oblivc/benchmark_scs.oc");
    println!("cargo:rerun-if-changed=src/oblivc/benchmark_common.h");
    println!("cargo:rerun-if-changed=src/oblivc/benchmark_common.c");
}
