//! extra stuff
use std::ffi::{CString, c_void};
use std::os::raw::{c_char, c_int};
use libc::{size_t, c_float};


#[link(name = "cbits")]
extern "C" {
    #[link_name = "SYS_EAGAIN"]
    fn sys_eagain() -> i32;
    #[link_name = "SYS_AVERROR"]
    fn sys_sys_averror(code: i32) -> i32;
    #[link_name = "SYS_AV_NOPTS_VALUE"]
    fn sys_av_nopts_value() -> i64;
    #[link_name = "SYS_AV_ERROR_MAX_STRING_SIZE"]
    fn av_error_max_string_size() -> c_int;
    #[link_name = "SYS_AVERROR_BSF_NOT_FOUND"]
    fn averror_bsf_not_found() -> c_int;
    #[link_name = "SYS_AVERROR_BUG"]
    fn averror_bug() -> c_int;
    #[link_name = "SYS_AVERROR_BUFFER_TOO_SMALL"]
    fn averror_buffer_too_small() -> c_int;
    #[link_name = "SYS_AVERROR_DECODER_NOT_FOUND"]
    fn averror_decoder_not_found() -> c_int;
    #[link_name = "SYS_AVERROR_DEMUXER_NOT_FOUND"]
    fn averror_demuxer_not_found() -> c_int;
    #[link_name = "SYS_AVERROR_ENCODER_NOT_FOUND"]
    fn averror_encoder_not_found() -> c_int;
    #[link_name = "SYS_AVERROR_EOF"]
    fn averror_eof() -> c_int;
    #[link_name = "SYS_AVERROR_EXIT"]
    fn averror_exit() -> c_int;
    #[link_name = "SYS_AVERROR_EXTERNAL"]
    fn averror_external() -> c_int;
    #[link_name = "SYS_AVERROR_FILTER_NOT_FOUND"]
    fn averror_filter_not_found() -> c_int;
    #[link_name = "SYS_AVERROR_INVALIDDATA"]
    fn averror_invaliddata() -> c_int;
    #[link_name = "SYS_AVERROR_MUXER_NOT_FOUND"]
    fn averror_muxer_not_found() -> c_int;
    #[link_name = "SYS_AVERROR_OPTION_NOT_FOUND"]
    fn averror_option_not_found() -> c_int;
    #[link_name = "SYS_AVERROR_PATCHWELCOME"]
    fn averror_patchwelcome() -> c_int;
    #[link_name = "SYS_AVERROR_PROTOCOL_NOT_FOUND"]
    fn averror_protocol_not_found() -> c_int;
    #[link_name = "SYS_AVERROR_STREAM_NOT_FOUND"]
    fn averror_stream_not_found() -> c_int;
}
