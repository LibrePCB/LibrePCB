//! Various helper functions

use regex::Regex;

/// Copy a string while incrementing its contained number
///
/// - If the string contains one or more numbers, the last one gets incremented
/// - If it does not contain a number, a "1" is appended instead
///
/// This way, the returned number is guaranteed to be different from the input
/// string. That's useful for example to generate unique, incrementing pin
/// numbers like "X1", "X2", "X3" etc.
///
/// # Arguments
///
/// * `s` - The input string.
///
/// # Returns
///
/// Returns a new string with the incremented number.
pub fn increment_number_in_string(s: &str) -> String {
  let mut ret = s.to_owned();

  let re = Regex::new(r"[0-9]+").unwrap();
  if let Some(mat) = re.find_iter(s).last() {
    // String contains numbers -> increment the last one.
    if let Ok(num) = mat.as_str().parse::<i32>() {
      ret.replace_range(mat.range(), &(num + 1).to_string());
      return ret;
    }
  }

  // Fallback: just add a "1" at the end.
  ret + "1"
}

#[cfg(test)]
mod tests {
  use super::*;

  create! {data, (input, expected), {
    assert_eq!(increment_number_in_string(input), expected);
  }}
  data! {
    d01: ("",                        "1"),
    d02: ("  ",                      "  1"),
    d03: ("0",                       "1"),
    d04: ("1",                       "2"),
    d05: (" 123 ",                   " 124 "),
    d06: ("X",                       "X1"),
    d07: ("X-1",                     "X-2"),
    d08: ("GND 41",                  "GND 42"),
    d09: ("FOO1.2",                  "FOO1.3"),
    d10: ("12 foo 34",               "12 foo 35"),
    d11: ("12 foo 34 bar 56 ",       "12 foo 34 bar 57 "),
    d12: ("99A",                     "100A"),
  }
}
