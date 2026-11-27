//! FFI for Ed25519 signature verification (used by the RPC server to
//! authenticate external clients, see the RPC server documentation).

use ed25519_compact::{PublicKey, Signature};

/// Verify an Ed25519 signature (see the RPC server documentation).
///
/// - `pubkey`: 32 raw bytes of the public key.
/// - `message`: The signed message (the nonce of the connection).
/// - `signature`: 64 raw bytes of the signature.
///
/// Returns whether the signature is valid.
#[no_mangle]
extern "C" fn ffi_ed25519_verify(
  pubkey: *const u8,
  message: *const u8,
  message_len: usize,
  signature: *const u8,
) -> bool {
  unsafe {
    let pubkey = std::slice::from_raw_parts(pubkey, 32);
    let message = std::slice::from_raw_parts(message, message_len);
    let signature = std::slice::from_raw_parts(signature, 64);
    let Ok(pubkey) = PublicKey::from_slice(pubkey) else {
      return false;
    };
    let Ok(signature) = Signature::from_slice(signature) else {
      return false;
    };
    pubkey.verify(message, &signature).is_ok()
  }
}
