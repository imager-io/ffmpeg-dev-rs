# WARNING

Not recommend for beginners. This library isn’t safe nor idiomatic rust, a consequence offering direct bindings to the FFmpeg C APIs. 

Furthermore:

* this library will complicate compilation
* this library will only be convenient for experienced Rust and C devs

# About
Direct, unobscured and self contained FFmpeg (sys) bindings.

By self contained I mean:
* Does not require or link against any FFmpeg system dependencies.
* Does not require a network connection for building.

**The FFmpeg bindings now include doc comments, including struct fields!** See [here](https://docs.rs/ffmpeg-dev/0.2.2/ffmpeg_dev/sys/avcodec/struct.AVCodec.html).

## Example

```rust
let input_path_cstr = std::ffi::CString::new("path/to/source.mp4").unwrap();

// Open an e.g. MP4 file
avformat_open_input(
    &mut ifmt_ctx,
    input_path_cstr.as_ptr(),
    std::ptr::null_mut(),
    std::ptr::null_mut(),
);
avformat_find_stream_info(ifmt_ctx, std::ptr::null_mut());

// Print info about the loaded file
av_dump_format(
    ifmt_ctx,
    0,
    input_path_cstr.as_ptr(),
    0,
);
```

For the uninitiated, the std includes lots of convenient ffi related utilities. E.g. using `std::slice::from_raw_parts`:
```rust
use ffmpeg_dev::sys::{
    AVMediaType_AVMEDIA_TYPE_VIDEO as AVMEDIA_TYPE_VIDEO,
    AVMediaType_AVMEDIA_TYPE_AUDIO as AVMEDIA_TYPE_AUDIO,
};
let ifmt_ctx: AVFormatContext = *ifmt_ctx;
let nb_streams = (*ifmt_ctx).nb_streams as usize;

// Extract video/audio/etc. streams from our mp4 file.
let streams = std::slice::from_raw_parts((*ifmt_ctx).streams, nb_streams)
    .iter()
    .map(|x| (*x).as_ref().expect("not null"))
    .collect::<Vec<&AVStream>>();

for (index, stream_ptr) in streams.iter().enumerate() {
    let codecpar = *stream_ptr.codecpar;
    if codecpar.codec_type == AVMEDIA_TYPE_AUDIO {
        println!("found audio stream at index {}", index);
    } else if codecpar.codec_type == AVMEDIA_TYPE_VIDEO {
        println!("found video stream at index {}", index);
    }
}
```

## Stability
API bindings should be **practically** stable now.

## Internal Behavior

By default the debug or dev builds compile FFmpeg without optimizations, this is for the purpose of speeding up compilation. Compiling on release mode or setting `opt-level` > 1 will disable this behavior.

# LICENSE WARNING
> I’m not a lawyer, furthermore I really don’t understand software licenses.
* This codebase is MIT.
* At compile time, this library builds and statically links against LGPL code.
    * This is for the purpose of being self contained, without burdening any library consumers with dependency issues.

Hopefully one day the rust ecosystem will get a decent FFmpeg alternative for e.g. container muxing/demuxing.

# Future
It would be interesting to experiment with compiling FFmpeg to WebAssembly. Perhaps as an alternative to static linking, if a local version isn’t available it could link to a remote lib over the network.

# Examples

```shell
$ cargo run --example h264_video_dec
```

## Added as of this writing
* `./examples/remux.rs`
* `./examples/h264_video_dec.rs`

# Miscellaneous

## RLS - Editor/IDE Issues

For some reason (as of this writing) RLS has issues with multiple versions of `ffmpeg-dev` downloaded (under `target`). If there’s something I can fix on my side I’m happy to implement such changes. For now I’m just resorting my deleting the cache folder whenever I update `ffmpeg-dev`.

## Miscellaneous Links:
* [FFmpeg docs overview](https://ffmpeg.org/documentation.html)
* [FFmpeg C API documentation](https://ffmpeg.org/doxygen/trunk/index.html)
* [FFmpeg C Examples](https://github.com/FFmpeg/FFmpeg/tree/master/doc/examples) (pretty easy to convert to rust in my experience)
* [Rust docs](https://docs.rs/ffmpeg-dev)

### Sample or Test Content

* [sintel_trailer-1080p.mp4](https://download.blender.org/durian/trailer/sintel_trailer-1080p.mp4)
* `./assets/test/test.h264` - heavily compressed version of the video stream from `sintel_trailer-1080p.mp4`. This is a raw H264 encoded video binary.

<hr/>

Built for [Imager](https://imager.io) - Site performance tools for efficiently distributing media on the web.
