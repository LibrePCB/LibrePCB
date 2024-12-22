//! FFI for [ibom][crate::ibom]

use super::cpp_ffi::*;
use crate::ibom::*;

/// Create a new [InteractiveHtmlBom] object
#[no_mangle]
pub extern "C" fn ffi_ibom_new() -> *mut InteractiveHtmlBom {
  let ibom = InteractiveHtmlBom::new();
  Box::into_raw(Box::new(ibom))
}

/// Delete [InteractiveHtmlBom] object
#[no_mangle]
pub extern "C" fn ffi_ibom_delete(obj: *mut InteractiveHtmlBom) {
  assert!(!obj.is_null());
  unsafe { drop(Box::from_raw(obj)) };
}

/// Wrapper for [generate]
#[no_mangle]
extern "C" fn ffi_ibom_generate(obj: &InteractiveHtmlBom, out: &mut QString) {
  let r_str = obj.generate();
  qstring_set(out, &r_str);
}
