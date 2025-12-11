#![no_std]
#![no_main]

use core::panic::PanicInfo;
use core::ffi::c_void;


const BUFFER_HEIGHT: usize = 25;
const BUFFER_WIDTH: usize = 80;

#[repr(transparent)]
struct Buffer {
    chars: [[u16; BUFFER_WIDTH]; BUFFER_HEIGHT],
}

static mut VGA_BUFFER: *mut Buffer = 0xb8000 as *mut Buffer;

#[no_mangle]
pub unsafe extern "C" fn os_print(msg: *const u8, x: usize, y: usize, color: u8) {
    let buffer = &mut *VGA_BUFFER;
    let mut idx = 0;
    
    loop {
        let char_byte = *msg.add(idx);
        if char_byte == 0 { break; }
        
        if x + idx < BUFFER_WIDTH && y < BUFFER_HEIGHT {
            buffer.chars[y][x + idx] = (color as u16) << 8 | (char_byte as u16);
        }
        idx += 1;
    }
}

#[no_mangle]
pub unsafe extern "C" fn os_cls(color: u8) {
    let buffer = &mut *VGA_BUFFER;
    for y in 0..BUFFER_HEIGHT {
        for x in 0..BUFFER_WIDTH {
            buffer.chars[y][x] = (color as u16) << 8 | (b' ' as u16);
        }
    }
}

extern "C" {
    fn user_main(); 
}

#[no_mangle]
pub extern "C" fn _start() -> ! {
    unsafe { os_cls(0x1F); }
    unsafe { user_main(); }

    loop {}
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

#[no_mangle]
pub unsafe extern "C" fn memcpy(dest: *mut u8, src: *const u8, n: usize) -> *mut u8 {
    let mut i = 0;
    while i < n {
        *dest.add(i) = *src.add(i);
        i += 1;
    }
    dest
}

#[no_mangle]
pub unsafe extern "C" fn memset(dest: *mut u8, c: i32, n: usize) -> *mut u8 {
    let mut i = 0;
    while i < n {
        *dest.add(i) = c as u8;
        i += 1;
    }
    dest
}

#[no_mangle]
pub unsafe extern "C" fn memcmp(s1: *const u8, s2: *const u8, n: usize) -> i32 {
    let mut i = 0;
    while i < n {
        if *s1.add(i) != *s2.add(i) {
            return (*s1.add(i) as i32) - (*s2.add(i) as i32);
        }
        i += 1;
    }
    0
}
