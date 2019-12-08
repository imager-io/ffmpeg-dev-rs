Self contained FFmpeg (sys) bindings.

Does not require or link against any FFmpeg system dependencies,
and does not require a network connection for building.

# LICENSE WARNING
> I’m not a lawyer, furthermore I really don’t understand software licenses.
* This codebase is MIT.
* At compile time, this library builds and statically links against LGPL code.
    * This is for the purpose of being self contained, without burdening any library consumers with dependency issues.

Hopefully one day the rust ecosystem will get a decent FFmpeg alternative for e.g. container muxing/demuxing.

# Future
It would be interesting to experiment with compiling FFmpeg to WebAssembly. Perhaps as an alternative to static linking it could link against either a local or remote LGPL binary.
