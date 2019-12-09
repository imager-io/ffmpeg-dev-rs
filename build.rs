#![allow(unused)]

use std::convert::AsRef;
use std::path::{PathBuf, Path};
use std::string::ToString;
use tar::Archive;
use flate2::read::GzDecoder;


///////////////////////////////////////////////////////////////////////////////
// UTILS - ENVIROMENT
///////////////////////////////////////////////////////////////////////////////

fn out_dir() -> PathBuf {
    PathBuf::from(std::env::var("OUT_DIR").expect("OUT_DIR env var"))
}

fn is_release_mode() -> bool {
    has_env_var_with_value("PROFILE", "release")
}

fn is_debug_mode() -> bool {
    has_env_var_with_value("PROFILE", "debug")
}

fn opt_level_eq(x: u8) -> bool {
    has_env_var_with_value("OPT_LEVEL", &format!("{}", x))
}

fn has_env_var_with_value(s: &str, v: &str) -> bool {
    std::env::var(s)
        .map(|x| x.to_lowercase())
        .map(|x| x == v.to_lowercase())
        .unwrap_or(false)
}

///////////////////////////////////////////////////////////////////////////////
// UTILS - BUILD
///////////////////////////////////////////////////////////////////////////////

pub fn extract_tar_file<P: AsRef<Path>, Q: AsRef<Path>>(tar_file: P, dest: Q) -> Result<(), String> {
    let source = std::fs::read(tar_file).expect("read tar file");
    let tar = GzDecoder::new(&source[..]);
    let mut archive = Archive::new(tar);
    // UNPACK ARCHIVE
    let tmp_source_dir: Option<PathBuf> = {
        archive
            .unpack(&dest)
            .map_err(|x| format!("[{:?}] failed to unpack tar file: {:?}", dest.as_ref(), x))?;
        let xs = std::fs::read_dir(&dest)
            .expect(&format!("unable to read dir {:?}", dest.as_ref()))
            .filter_map(Result::ok)
            .filter(|file| file.file_type().map(|x| x.is_dir()).unwrap_or(false))
            .collect::<Vec<std::fs::DirEntry>>();
        match &xs[..] {
            [x] => Some(x.path()),
            _ => None,
        }
    };
    Ok(())
}

pub fn lookup_newest(paths: Vec<PathBuf>) -> Option<PathBuf> {
    use std::time::{SystemTime, Duration};
    let mut newest: Option<(PathBuf, Duration)> = None;
    paths
        .clone()
        .into_iter()
        .filter_map(|x: PathBuf| {
            let timestamp = x
                .metadata()
                .ok()
                .and_then(|y| y.created().ok())
                .and_then(|x| x.duration_since(SystemTime::UNIX_EPOCH).ok());
            match timestamp {
                Some(y) => Some((x, y)),
                _ => None
            }
        })
        .for_each(|(x_path, x_created)| match &newest {
            None => {
                newest = Some((x_path, x_created));
            }
            Some((_, y_created)) => {
                if &x_created > y_created {
                    newest = Some((x_path, x_created));
                }
            }
        });
    // DONE
    newest.map(|(x, _)| x)
}

pub fn files_with_prefix(dir: &PathBuf, pattern: &str) -> Vec<PathBuf> {
    std::fs::read_dir(dir)
        .expect(&format!("get dir contents: {:?}", dir))
        .filter_map(Result::ok)
        .filter_map(|x| {
            let file_name = x
                .file_name()
                .to_str()?
                .to_owned();
            if file_name.starts_with(pattern) {
                Some(x.path())
            } else {
                None
            }
        })
        .collect::<Vec<_>>()
}

fn run_make(source_path: &PathBuf, makefile: &str) {
    let result = std::process::Command::new("make")
        .arg("-C")
        .arg(source_path)
        .arg("-f")
        .arg(makefile)
        .output()
        .expect(&format!("make -C {:?} failed", source_path));
    assert!(result.status.success());
}

fn cpy<P: AsRef<Path>, Q: AsRef<Path>>(from: P, to: Q) {
    std::fs::copy(&from, &to)
        .expect(&format!(
            "unable to cpy from {:?} to {:?}",
            from.as_ref(),
            to.as_ref(),
        ));
}

///////////////////////////////////////////////////////////////////////////////
// PATHS
///////////////////////////////////////////////////////////////////////////////

pub const STATIC_LIBS: &[(&str, &str)] = &[
    (
        "avcodec",
        "libavcodec/libavcodec.a",
    ),
    (
        "avdevice",
        "libavdevice/libavdevice.a",
    ),
    (
        "avfilter",
        "libavfilter/libavfilter.a",
    ),
    (
        "avformat",
        "libavformat/libavformat.a",
    ),
    (
        "avutil",
        "libavutil/libavutil.a",
    ),
    (
        "swresample",
        "libswresample/libswresample.a",
    ),
    (
        "swscale",
        "libswscale/libswscale.a",
    ),
];

pub const HEADER_GROUPS: &[(&str, &[&str])] = &[
    (
        "avcodec",
        &[
            "libavcodec/avcodec.h",
        ]
    ),
    (
        "avdevice",
        &[
            "libavdevice/avdevice.h",
        ]
    ),
    (
        "avfilter",
        &[
            "libavfilter/avfilter.h",
        ]
    ),
    (
        "avformat",
        &[
            "libavformat/avformat.h",
        ]
    ),
    (
        "avresample",
        &[
            "libavresample/avresample.h",
        ]
    ),
    (
        "avutil",
        &[
            "libavutil/avutil.h",
        ]
    ),
    (
        "swresample",
        &[
            "libswresample/swresample.h",
        ]
    ),
    (
        "swscale",
        &[
            "libswscale/swscale.h",
        ]
    ),
];

pub const SEARCH_PATHS: &[&str] = &[
    "libavcodec",
    "libavdevice",
    "libavfilter",
    "libavformat",
    "libavresample",
    "libavutil",
    "libpostproc",
    "libswresample",
    "libswscale",
];

///////////////////////////////////////////////////////////////////////////////
// BUILD PIPELINE
///////////////////////////////////////////////////////////////////////////////

fn build() {
    let out_path = out_dir();
    let source_path = out_path.join("FFmpeg-FFmpeg-2722fc2");
    // SPEED UP DEV - UNLESS IN RELASE MODE
    let already_built = {
        STATIC_LIBS
            .iter()
            .map(|(_, x)| source_path.join(x))
            .all(|x| x.exists())
    };
    let mut skip_build = already_built && !is_release_mode();
    if has_env_var_with_value("FFDEV1", "1") {
        skip_build = false;
    }
    // EXTRACT
    if !source_path.exists() || !skip_build {
        // extract_tar_file("archive/FFmpeg-FFmpeg-2722fc2.tar.gz", &out_path);
        {
            let result = std::process::Command::new("tar")
                .arg("-xJf")
                .arg("archive/FFmpeg-FFmpeg-2722fc2.tar.xz")
                .arg("-C")
                .arg(out_path.to_str().expect("PathBuf to str"))
                .output()
                .expect("tar decompression of ffmpeg source repo using xz (to fit the 10M crates limit)");
            assert!(result.status.success());
        }
        assert!(source_path.exists());  
    }
    // BUILD CODE PHASE
    if skip_build == false {
        // CONFIGURE
        {
            let mut configure_flags = vec![
                "--disable-programs",
                "--disable-doc",
                "--disable-autodetect",
            ];
            // TRY TO SPEED THIS UP FOR DEV BUILDS
            if is_debug_mode() && opt_level_eq(0) {
                configure_flags.push("--disable-optimizations");
                configure_flags.push("--disable-debug");
                configure_flags.push("--disable-stripping");
            }
            let eval_configure = |flags: Vec<&str>| {
                let flags = flags.join(" ");
                std::process::Command::new("sh")
                    .arg("-c")
                    .arg(&format!(
                        "cd {path} && ./configure {flags}",
                        path=source_path.to_str().expect("PathBuf to str"),
                        flags=flags,
                    ))
                    .output()
                    .expect(&format!("ffmpeg configure script"))
            };
            let result = eval_configure(configure_flags.clone());
            if !result.status.success() {
                let stderr = String::from_utf8(result.stderr).expect("invalid str");
                let stdout = String::from_utf8(result.stdout).expect("invalid str");
                let nasm_yasm_issue = stderr
                    .lines()
                    .any(|x| x.contains("nasm/yasm not found or too old"));
                // MAYBE RETRY (USE CRIPPLED BUILD)
                if nasm_yasm_issue {
                    configure_flags.push("--disable-x86asm");
                    let result = eval_configure(configure_flags);
                    if !result.status.success() {
                        let stderr = String::from_utf8(result.stderr).expect("invalid str");
                        let stdout = String::from_utf8(result.stdout).expect("invalid str");
                        panic!("configure failed:\n{}", vec![stderr, stdout].join("\n"));
                    }
                } else {
                    panic!("configure failed:\n{}", vec![stderr, stdout].join("\n"));
                }
            }
        }
        // BUILD
        {
            let mut cpu_number = num_cpus::get();
            let result = std::process::Command::new("make")
                .arg("-C")
                .arg(&source_path)
                .arg("-f")
                .arg("Makefile")
                .arg(&format!("-j{}", cpu_number))
                .output()
                .expect(&format!("make -C {:?} failed", source_path));
            assert!(result.status.success());
        }
    }
    // LINK
    for path in SEARCH_PATHS {
        println!("cargo:rustc-link-search=native={}", {
            source_path.join(path).to_str().expect("PathBuf as str")
        });
    }
    for (name, _) in STATIC_LIBS {
        println!("cargo:rustc-link-lib=static={}", name);
    }
    // CODEGEN SETUP
    let skip_codegen = HEADER_GROUPS
        .iter()
        .map(|(x, _)| format!("bindings_{}.rs", x))
        .map(|x| out_path.join(x))
        .all(|x| x.exists());
    // CODEGEN
    let codegen = |file_name: &str, headers: &[&str]| {
        let codegen = bindgen::Builder::default();
        let codegen = headers
            .iter()
            .fold(codegen, |codegen: bindgen::Builder, path: &&str| -> bindgen::Builder {
                let path: &str = path.clone();
                let path: PathBuf = source_path.join(path);
                let path: &str = path.to_str().expect("PathBuf to str");
                assert!(PathBuf::from(path).exists());
                codegen.header(path)
            });
        codegen
            .generate_comments(true)
            .generate()
            .expect("Unable to generate bindings")
            .write_to_file(out_path.join(file_name))
            .expect("Couldn't write bindings!");
    };
    if !skip_codegen {
        for (name, hs) in HEADER_GROUPS {
            codegen(&format!("bindings_{}.rs", name), hs);
        }   
    }
}

///////////////////////////////////////////////////////////////////////////////
// MAIN
///////////////////////////////////////////////////////////////////////////////

fn main() {
    build();
}
