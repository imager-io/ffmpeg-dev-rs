//! Function wrappers around global C macros.
use std::ffi::{CString, c_void};
use std::os::raw::{c_char, c_int};
use libc::{size_t, c_float};

#[link(name = "cbits")]
extern "C" {
    #[link_name = "SYS_EAGAIN"]
    pub fn eagain() -> i32;
    #[link_name = "SYS_AVERROR"]
    pub fn averror(code: i32) -> i32;
    #[link_name = "SYS_AV_NOPTS_VALUE"]
    pub fn av_nopts_value() -> i64;
    #[link_name = "SYS_AV_ERROR_MAX_STRING_SIZE"]
    pub fn av_error_max_string_size() -> c_int;
    #[link_name = "SYS_AVERROR_BSF_NOT_FOUND"]
    pub fn averror_bsf_not_found() -> c_int;
    #[link_name = "SYS_AVERROR_BUG"]
    pub fn averror_bug() -> c_int;
    #[link_name = "SYS_AVERROR_BUFFER_TOO_SMALL"]
    pub fn averror_buffer_too_small() -> c_int;
    #[link_name = "SYS_AVERROR_DECODER_NOT_FOUND"]
    pub fn averror_decoder_not_found() -> c_int;
    #[link_name = "SYS_AVERROR_DEMUXER_NOT_FOUND"]
    pub fn averror_demuxer_not_found() -> c_int;
    #[link_name = "SYS_AVERROR_ENCODER_NOT_FOUND"]
    pub fn averror_encoder_not_found() -> c_int;
    #[link_name = "SYS_AVERROR_EOF"]
    pub fn averror_eof() -> c_int;
    #[link_name = "SYS_AVERROR_EXIT"]
    pub fn averror_exit() -> c_int;
    #[link_name = "SYS_AVERROR_EXTERNAL"]
    pub fn averror_external() -> c_int;
    #[link_name = "SYS_AVERROR_FILTER_NOT_FOUND"]
    pub fn averror_filter_not_found() -> c_int;
    #[link_name = "SYS_AVERROR_INVALIDDATA"]
    pub fn averror_invaliddata() -> c_int;
    #[link_name = "SYS_AVERROR_MUXER_NOT_FOUND"]
    pub fn averror_muxer_not_found() -> c_int;
    #[link_name = "SYS_AVERROR_OPTION_NOT_FOUND"]
    pub fn averror_option_not_found() -> c_int;
    #[link_name = "SYS_AVERROR_PATCHWELCOME"]
    pub fn averror_patchwelcome() -> c_int;
    #[link_name = "SYS_AVERROR_PROTOCOL_NOT_FOUND"]
    pub fn averror_protocol_not_found() -> c_int;
    #[link_name = "SYS_AVERROR_STREAM_NOT_FOUND"]
    pub fn averror_stream_not_found() -> c_int;
}
