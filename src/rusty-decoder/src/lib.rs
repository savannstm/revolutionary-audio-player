use encoding_rs::WINDOWS_1251;
use std::ffi::{CStr, CString};
use std::os::raw::c_char;

#[no_mangle]
pub unsafe extern "C" fn decode1251(string: *const c_char) -> *const c_char {
    if string.is_null() {
        return std::ptr::null();
    }

    let c_str: &CStr = unsafe { CStr::from_ptr(string) };
    let str_slice: &str = c_str.to_str().unwrap_or("");

    let (decoded, _, had_errors) = WINDOWS_1251.decode(str_slice.as_bytes());
    let string = CString::new(decoded.into_owned()).unwrap();
    string.as_ptr()
}
