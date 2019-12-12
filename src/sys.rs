//! Links:
//! * [FFmpeg docs overview](https://ffmpeg.org/documentation.html)
//! * [FFmpeg C API documentation](https://ffmpeg.org/doxygen/trunk/index.html)
//! * [Rust docs](https://docs.rs/ffmpeg-dev)
//! * [C Examples](https://github.com/FFmpeg/FFmpeg/tree/master/doc/examples) (Pretty easy to convert to rust in my experience.)
#![allow(unused)]
#![allow(non_snake_case)]
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(improper_ctypes)]
#![allow(safe_packed_borrows)]

include!(concat!(env!("OUT_DIR"), "/bindings_ffmpeg.rs"));

// pub mod avcodec {
//     include!(concat!(env!("OUT_DIR"), "/bindings_avcodec.rs"));
// }
// pub mod avdevice {
//     include!(concat!(env!("OUT_DIR"), "/bindings_avdevice.rs"));
// }
// // pub mod avfilter {
// //     include!(concat!(env!("OUT_DIR"), "/bindings_avfilter.rs"));
// // }
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