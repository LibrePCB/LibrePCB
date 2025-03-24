//! FFI for
//! [`zip::ZipWriter`](https://docs.rs/zip/0.6.6/zip/write/struct.ZipWriter.html)

use super::cpp_ffi::*;
use std::fs;
use std::fs::File;
use std::io::Write;

/// Proxy type for in-memory and file-based
/// [`zip::ZipWriter`](https://docs.rs/zip/0.6.6/zip/write/struct.ZipWriter.html)
enum Writer {
  /// File-based Zip archive
  File(zip::ZipWriter<File>),
  /// In-memory Zip archive
  Mem(zip::ZipWriter<QByteArrayWriter>),
}

/// Wrapper type for [Writer]
struct ZipWriter(Writer);

/// Create a new [ZipWriter] object writing to a file
#[no_mangle]
extern "C" fn ffi_zipwriter_new_to_file(
  path: &QString,
  err: &mut QString,
) -> *mut ZipWriter {
  let path_str = from_qstring(path);
  let fp = std::path::Path::new(&path_str);
  if let Some(parent) = fp.parent() {
    if !parent.exists() {
      _ = fs::create_dir_all(parent);
    }
  }
  let file = match File::create(fp) {
    Ok(file) => file,
    Err(e) => {
      qstring_set(err, e.to_string().as_str());
      return std::ptr::null_mut();
    }
  };
  let zip = zip::ZipWriter::new(file);
  Box::into_raw(Box::new(ZipWriter(Writer::File(zip))))
}

/// Create a new [ZipWriter] object writing to memory
#[no_mangle]
extern "C" fn ffi_zipwriter_new_to_mem(
  data: &mut QByteArray,
) -> *mut ZipWriter {
  let writer = QByteArrayWriter::new(data);
  let zip = zip::ZipWriter::new(writer);
  Box::into_raw(Box::new(ZipWriter(Writer::Mem(zip))))
}

/// Delete [ZipWriter] object
#[no_mangle]
extern "C" fn ffi_zipwriter_delete(obj: *mut ZipWriter) {
  assert!(!obj.is_null());
  unsafe { drop(Box::from_raw(obj)) };
}

/// Write a file to [ZipWriter]
#[no_mangle]
extern "C" fn ffi_zipwriter_write_file(
  obj: &mut ZipWriter,
  name: &QString,
  data: &QByteArray,
  mode: u32,
  err: &mut QString,
) -> bool {
  let options = zip::write::FileOptions::default()
    .compression_method(zip::CompressionMethod::Deflated)
    .unix_permissions(mode);
  let start_res = match &mut obj.0 {
    Writer::File(zip) => zip.start_file(from_qstring(name), options),
    Writer::Mem(zip) => zip.start_file(from_qstring(name), options),
  };
  if let Err(e) = start_res {
    qstring_set(err, e.to_string().as_str());
    return false;
  }
  let write_res = match &mut obj.0 {
    Writer::File(zip) => zip.write_all(qbytearray_to_slice(data)),
    Writer::Mem(zip) => zip.write_all(qbytearray_to_slice(data)),
  };
  if let Err(e) = write_res {
    qstring_set(err, e.to_string().as_str());
    return false;
  }
  true
}

/// Finish writing to [ZipWriter]
#[no_mangle]
extern "C" fn ffi_zipwriter_finish(
  obj: &mut ZipWriter,
  err: &mut QString,
) -> bool {
  let res = match &mut obj.0 {
    Writer::File(zip) => zip.finish().err(),
    Writer::Mem(zip) => zip.finish().err(),
  };
  if let Some(e) = res {
    qstring_set(err, e.to_string().as_str());
    return false;
  }
  true
}
