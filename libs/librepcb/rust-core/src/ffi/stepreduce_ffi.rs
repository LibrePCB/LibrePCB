//! FFI for
//! [`interactive-html-bom::InteractiveHtmlBom`](https://docs.rs/interactive-html-bom/latest/struct.interactiveHtmlBom.html)

use super::cpp_ffi::*;
use stepreduce::*;

/// Wrapper for [increment_number_in_string]
#[no_mangle]
extern "C" fn ffi_stepreduce(input: &mut QByteArray) {
  let opt = ReduceOptions {
    max_decimals: None,
    use_step_precision: false,
  };
  let reduced = reduce(qbytearray_to_slice(input), &opt);
  qbytearray_set(input, &reduced);
}
