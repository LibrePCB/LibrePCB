//! FFI for
//! [`zip::ZipArchive`](https://docs.rs/zip/0.6.6/zip/read/struct.ZipArchive.html)

use super::cpp_ffi::*;
use std::fs::File;
use std::io::{Cursor, Read};

/// Proxy type for in-memory and file-based
/// [`zip::ZipArchive`](https://docs.rs/zip/0.6.6/zip/read/struct.ZipArchive.html)
enum Archive {
  /// File-based Zip archive
  File(zip::ZipArchive<File>),
  /// In-memory Zip archive
  Mem(zip::ZipArchive<Cursor<Vec<u8>>>),
}

/// Wrapper type for [Archive]
struct ZipArchive(Archive);

/// Create a new [ZipArchive] object from file path
#[no_mangle]
extern "C" fn ffi_ziparchive_new_from_file(
  path: &QString,
  err: &mut QString,
) -> *mut ZipArchive {
  let file = match File::open(std::path::Path::new(&from_qstring(path))) {
    Ok(file) => file,
    Err(e) => {
      qstring_set(err, e.to_string().as_str());
      return std::ptr::null_mut();
    }
  };
  match zip::ZipArchive::new(file) {
    Ok(zip) => Box::into_raw(Box::new(ZipArchive(Archive::File(zip)))),
    Err(e) => {
      qstring_set(err, e.to_string().as_str());
      std::ptr::null_mut()
    }
  }
}

/// Create a new [ZipArchive] object from memory
#[no_mangle]
extern "C" fn ffi_ziparchive_new_from_mem(
  data: &QByteArray,
  err: &mut QString,
) -> *mut ZipArchive {
  let reader = Cursor::new(from_qbytearray(data).to_vec());
  match zip::ZipArchive::new(reader) {
    Ok(zip) => Box::into_raw(Box::new(ZipArchive(Archive::Mem(zip)))),
    Err(e) => {
      qstring_set(err, e.to_string().as_str());
      std::ptr::null_mut()
    }
  }
}

/// Delete [ZipArchive] object
#[no_mangle]
extern "C" fn ffi_ziparchive_delete(obj: *mut ZipArchive) {
  assert!(!obj.is_null());
  unsafe { drop(Box::from_raw(obj)) };
}

/// Get number of files in [ZipArchive]
#[no_mangle]
extern "C" fn ffi_ziparchive_len(obj: &ZipArchive) -> usize {
  match &obj.0 {
    Archive::File(zip) => zip.len(),
    Archive::Mem(zip) => zip.len(),
  }
}

/// Get name of a file in [ZipArchive]
#[no_mangle]
extern "C" fn ffi_ziparchive_name_for_index(
  obj: &mut ZipArchive,
  index: usize,
  name: &mut QString,
  err: &mut QString,
) -> bool {
  let res = match &mut obj.0 {
    Archive::File(zip) => zip.by_index(index),
    Archive::Mem(zip) => zip.by_index(index),
  };
  match res {
    Ok(f) => match f.enclosed_name() {
      Some(path) if path.to_str().is_some() => {
        qstring_set(name, path.to_str().unwrap());
        true
      }
      _ => {
        qstring_set(err, "Zip contains invalid or unsafe paths.");
        false
      }
    },
    Err(e) => {
      qstring_set(err, e.to_string().as_str());
      false
    }
  }
}

/// Read a file from [ZipArchive]
#[no_mangle]
extern "C" fn ffi_ziparchive_read_by_index(
  obj: &mut ZipArchive,
  index: usize,
  buf: &mut QByteArray,
  err: &mut QString,
) -> bool {
  let res = match &mut obj.0 {
    Archive::File(zip) => zip.by_index(index),
    Archive::Mem(zip) => zip.by_index(index),
  };
  match res {
    Ok(mut f) => {
      qbytearray_resize(buf, f.size() as usize, 0);
      if let Err(e) = f.read_exact(qbytearray_to_slice_mut(buf)) {
        qstring_set(err, e.to_string().as_str());
        false
      } else {
        true
      }
    }
    Err(e) => {
      qstring_set(err, e.to_string().as_str());
      false
    }
  }
}

/// Extract [ZipArchive] to directory
#[no_mangle]
extern "C" fn ffi_ziparchive_extract(
  obj: &mut ZipArchive,
  dir: &QString,
) -> bool {
  let dir_str = &from_qstring(dir);
  let path = std::path::Path::new(dir_str);
  match &mut obj.0 {
    Archive::File(zip) => zip.extract(path),
    Archive::Mem(zip) => zip.extract(path),
  }
  .is_ok()
}
