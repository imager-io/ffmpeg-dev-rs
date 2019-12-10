Direct, unobscured and self contained FFmpeg (sys) bindings.

```rust
let ifmt_ctx: AVFormatContext = *ifmt_ctx;

// Extract video/audio/etc. streams from an e.g. MP4 file
let streams: &[AVStream] = std::slice::from_raw_parts(
    *ifmt_ctx.streams, 
    ifmt_ctx.nb_streams as usize
);

// C bindings require zero for` loops 😌 - instead turn C dynamic arrays into Rust array refs
for stream in std::slice::from_raw_parts(*ifmt_ctx.streams, ifmt_ctx.nb_streams as usize) {
    /// ... stream is of type '&AVStream'
}
```

By self contained I mean:
* Does not require or link against any FFmpeg system dependencies.
* Does not require a network connection for building.

**The FFmpeg bindings now include doc comments, including struct fields!** See [here](https://docs.rs/ffmpeg-dev/0.2.2/ffmpeg_dev/sys/avcodec/struct.AVCodec.html).

# Stability
API bindings should be **practically** stable now.

# Internal Behavior

By default the debug or dev builds compile FFmpeg without optimizations, this is for the purpose of speeding up compilation. Compiling on release mode or setting `opt-level` > 1 will disable this behavior.

# LICENSE WARNING
> I’m not a lawyer, furthermore I really don’t understand software licenses.
* This codebase is MIT.
* At compile time, this library builds and statically links against LGPL code.
    * This is for the purpose of being self contained, without burdening any library consumers with dependency issues.

Hopefully one day the rust ecosystem will get a decent FFmpeg alternative for e.g. container muxing/demuxing.

# Future
It would be interesting to experiment with compiling FFmpeg to WebAssembly. Perhaps as an alternative to static linking, if a local version isn’t available it could link to a remote lib over the network.

<hr/>

Built for [Imager](https://imager.io) - Site performance tools for efficiently distributing media on the web.
