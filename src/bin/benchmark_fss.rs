extern crate oblivc;
extern crate liback_sys;
extern crate time;

pub mod fss_oblivc {
    #![allow(non_snake_case, non_camel_case_types, non_upper_case_globals, unused_variables)]
    include!(concat!(env!("OUT_DIR"), "/benchmark_fss.rs"));
}
use self::fss_oblivc::*;

fn main() {
    for len in (10..23).map(|x| 1<<x) {
        println!("Running len = {}", len);
        let t_start = time::now();
        let len2 = len;
        let server = ::std::thread::spawn(move || {
            let mut args = BenchmarkFSSArgs{
                len: len2,
            };
            let pd = oblivc::protocol_desc()
                .party(1)
                .accept(format!("{}", 37845)).unwrap();
            unsafe {
                pd.exec_yao_protocol(benchmark_fss, &mut args);
            }
        });
        let mut args = BenchmarkFSSArgs{
            len: len,
        };
        let pd = oblivc::protocol_desc()
            .party(2)
            .connect("localhost", format!("{}", 37845)).unwrap();
        unsafe {
            pd.exec_yao_protocol(benchmark_fss, &mut args);
        }
        server.join().unwrap();
        let msecs = (time::now() - t_start).num_milliseconds();
        println!("Time (ms): {}", msecs);
    }
}
