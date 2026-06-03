//! FFI for
//! [`keyring-core`](https://docs.rs/keyring-core/latest/keyring_core/index.html)

use super::cpp_ffi::*;
use keyring_core::Entry;

#[no_mangle]
extern "C" fn ffi_keyring_init() -> bool {
  if let Ok(store) = dbus_secret_service_keyring_store::Store::new() {
    keyring_core::set_default_store(store);
    return true;
  }
  false
}

#[no_mangle]
extern "C" fn ffi_keyring_set_password(
  service: &QString,
  user: &QString,
  password: &QString,
) -> bool {
  if let Ok(entry) = Entry::new(&from_qstring(service), &from_qstring(user)) {
    return entry.set_password(&from_qstring(password)).is_ok();
  }
  false
}

#[no_mangle]
extern "C" fn ffi_keyring_get_password(
  service: &QString,
  user: &QString,
  password: &mut QString,
) -> bool {
  if let Ok(entry) = Entry::new(&from_qstring(service), &from_qstring(user)) {
    if let Ok(pw) = entry.get_password() {
      qstring_set(password, &pw);
      return true;
    }
  }
  false
}

#[no_mangle]
extern "C" fn ffi_keyring_delete_credentials(
  service: &QString,
  user: &QString,
) -> bool {
  if let Ok(entry) = Entry::new(&from_qstring(service), &from_qstring(user)) {
    return entry.delete_credential().is_ok();
  }
  false
}
