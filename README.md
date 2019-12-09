Self contained FFmpeg (sys) bindings.

Does not require or link against any FFmpeg system dependencies,
and does not require a network connection for building.

**The FFmpeg bindings now include doc comments, including struct fields!** See [here](https://docs.rs/ffmpeg-dev/0.2.2/ffmpeg_dev/sys/avcodec/struct.AVCodec.html).

# NOTE

For the current version (`2.0`), I’m currently having issues with `docs.rs` (and maybe linux builds in general). Will publish the working, somewhat stable version on `0.3`.

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
