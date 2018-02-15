extern crate oblivc;
extern crate liback_sys;
// extern crate mvpir;

pub mod fss_oblivc {
    #![allow(non_snake_case, non_camel_case_types, non_upper_case_globals, unused_variables)]
    include!(concat!(env!("OUT_DIR"), "/benchmark_fss.rs"));
}
use self::fss_oblivc::*;

fn main() {
    let server = ::std::thread::spawn(|| {
        let mut args = BenchmarkFSSArgs{};
        let pd = oblivc::protocol_desc()
            .party(1)
            .accept(format!("{}", 67845)).unwrap();
        unsafe {
            pd.exec_yao_protocol(benchmark_fss, &mut args);
        }
    });
    let mut args = BenchmarkFSSArgs{};
    let pd = oblivc::protocol_desc()
        .party(2)
        .connect("localhost", format!("{}", 67845)).unwrap();
    unsafe {
        pd.exec_yao_protocol(benchmark_fss, &mut args);
    }
    server.join().unwrap();
}
