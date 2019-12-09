#![allow(unused)]
#![allow(non_snake_case)]
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(improper_ctypes)]


pub mod avcodec {
    include!(concat!(env!("OUT_DIR"), "/bindings_avcodec.rs"));
}
// pub mod avdevice {
//     include!(concat!(env!("OUT_DIR"), "/bindings_avdevice.rs"));
// }
// pub mod avfilter {
//     include!(concat!(env!("OUT_DIR"), "/bindings_avfilter.rs"));
// }
// pub mod avformat {
//     include!(concat!(env!("OUT_DIR"), "/bindings_avformat.rs"));
// }
// pub mod avresample {
//     include!(concat!(env!("OUT_DIR"), "/bindings_avresample.rs"));
// }
// pub mod avutil {
//     include!(concat!(env!("OUT_DIR"), "/bindings_avutil.rs"));
// }
// pub mod swresample {
//     include!(concat!(env!("OUT_DIR"), "/bindings_swresample.rs"));
// }
// pub mod swscale {
//     include!(concat!(env!("OUT_DIR"), "/bindings_swscale.rs"));
// }