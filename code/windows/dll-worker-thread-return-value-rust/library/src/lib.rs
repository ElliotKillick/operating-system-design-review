// We use module constructors and destructors because:
// 1. We want to avoid lazy initialization
//   - Rust is a system-level programming language and we do not want our system-level code to incur the overhead that comes with constantly evaluating an atomic or synchronized "is initialized" condition as is the case with a Rust lazy_static or once_cell
//   - Our subsystem should be fully initialized when it is done loading
// 2. Our module is one that presents significant cross-cutting concerns
//   - As a result, we need a predictable initialization time to avoid unexpected results
// 3. Timing matters
//   - Some initialization tasks require I/O, which could lead to priority inversion if they run at an unexpeted time
//   - Managing priority is especially important when building a real-time application
//   - Timing differences due to a lazy initialization slowdown can be used in a timing attack as a side-channel for leaking information
// 4. Two-phase initialization is an anti-pattern and applying it to the module scope comes with particularly poor outcomes
//   - Constructors should be able to construct
//   - The same can reasonably extend to lazy initialization, although lazy initialization trades composability issues and a partial initialization danger for a perfomance hit by constantly checking for initialization on every use
// 5. Our library has a worker thread that must be joined to exit cleanly
//   - This is reasonable resource for a library to own, Microsoft documentation: "Consider a DLL that creates worker threads as part of its initialization."
// 6. Operating systems should be sane
//   - It is not like any operating system would be broken and unhinged enough to go around terminating threads, right? Right?

use std::sync::Mutex;
use std::thread::{self, JoinHandle};

// Note that the ctor package is bugged in that it does not provide true destructor access, rather relying on atexit which may be different from a syncrhonization point of view (but on Windows, library atexit is run in the module destructor, so it makes no difference for our purposes)
// The author is aware of this issue: https://github.com/mmastrac/rust-ctor/issues/284
use ctor::{ctor, dtor};

// Create a worker thread object that returns an integer in the module scope
static WORKER: Mutex<Option<JoinHandle<i32>>> = Mutex::new(None);

#[ctor]
fn my_constructor() {
    println!("Module constructor called!");
    let worker_thread = thread::spawn(|| {
        // Worker thread does some work, it could be anything, and in the end it returns a value
        println!("Started thread.");
        // Doing some arbitrary work
        thread::sleep(std::time::Duration::from_secs(10));
        println!("Thread finished.");
        21 + 21
    });
    *WORKER.lock().unwrap() = Some(worker_thread);
    // Allow thread time to startup, just because
    thread::sleep(std::time::Duration::from_secs(5));
}

#[dtor]
fn my_destructor() {
    // Do not print a message here because we could deadlock on the Rust runtime's stdio lock if it became orphaned
    //println!("Module destructor called!");
    if let Some(worker_thread) = WORKER.lock().unwrap().take() {
        //panic!("Checkpoint");
        // We crash here!
        let result = worker_thread.join().unwrap();
        // Use panic as a hack to print an unsynchronized message
        panic!("Thread joined, result: {}", result);
        // Do not print a message here because we could deadlock on the Rust runtime's stdio lock if it became orphaned
        //println!("Thread joined, result: {}", result);
    }
}

pub fn hello_world() {
    println!("Hello, World from library!");
}

// Result:
/*
Module constructor called!
Hello, World from library!
Started thread.

thread 'main' panicked at /rustc/4d91de4e48198da2e33413efdcd9cd2cc0c46688\library\std\src\thread\mod.rs:1732:40:
called `Option::unwrap()` on a `None` value
note: run with `RUST_BACKTRACE=1` environment variable to display a backtrace

thread 'main' panicked at library\core\src\panicking.rs:218:5:
panic in a function that cannot unwind
stack backtrace:
   0:     0x7fffeedb7211 - std::backtrace_rs::backtrace::dbghelp64::trace
                               at /rustc/4d91de4e48198da2e33413efdcd9cd2cc0c46688/library\std\src\..\..\backtrace\src\backtrace\dbghelp64.rs:91
   1:     0x7fffeedb7211 - std::backtrace_rs::backtrace::trace_unsynchronized
                               at /rustc/4d91de4e48198da2e33413efdcd9cd2cc0c46688/library\std\src\..\..\backtrace\src\backtrace\mod.rs:66
   2:     0x7fffeedb7211 - std::sys::backtrace::_print_fmt
                               at /rustc/4d91de4e48198da2e33413efdcd9cd2cc0c46688/library\std\src\sys\backtrace.rs:66
   3:     0x7fffeedb7211 - std::sys::backtrace::impl$0::print::impl$0::fmt
                               at /rustc/4d91de4e48198da2e33413efdcd9cd2cc0c46688/library\std\src\sys\backtrace.rs:39
   4:     0x7fffeede9d6a - core::fmt::rt::Argument::fmt
                               at /rustc/4d91de4e48198da2e33413efdcd9cd2cc0c46688/library\core\src\fmt\rt.rs:177
   5:     0x7fffeede9d6a - core::fmt::write
                               at /rustc/4d91de4e48198da2e33413efdcd9cd2cc0c46688/library\core\src\fmt\mod.rs:1440
   6:     0x7fffeedacb57 - std::io::Write::write_fmt<std::sys::pal::windows::stdio::Stderr>
                               at /rustc/4d91de4e48198da2e33413efdcd9cd2cc0c46688/library\std\src\io\mod.rs:1887
   7:     0x7fffeedb7055 - std::sys::backtrace::BacktraceLock::print
                               at /rustc/4d91de4e48198da2e33413efdcd9cd2cc0c46688/library\std\src\sys\backtrace.rs:42
   8:     0x7fffeedb9ffd - std::panicking::default_hook::closure$1
                               at /rustc/4d91de4e48198da2e33413efdcd9cd2cc0c46688/library\std\src\panicking.rs:295
   9:     0x7fffeedb9d60 - std::panicking::default_hook
                               at /rustc/4d91de4e48198da2e33413efdcd9cd2cc0c46688/library\std\src\panicking.rs:322
  10:     0x7fffeedba8af - std::panicking::rust_panic_with_hook
                               at /rustc/4d91de4e48198da2e33413efdcd9cd2cc0c46688/library\std\src\panicking.rs:828
  11:     0x7fffeedba612 - std::panicking::begin_panic_handler::closure$0
                               at /rustc/4d91de4e48198da2e33413efdcd9cd2cc0c46688/library\std\src\panicking.rs:694
  12:     0x7fffeedb7cef - std::sys::backtrace::__rust_end_short_backtrace<std::panicking::begin_panic_handler::closure_env$0,never$>
                               at /rustc/4d91de4e48198da2e33413efdcd9cd2cc0c46688/library\std\src\sys\backtrace.rs:168
  13:     0x7fffeedba24e - std::panicking::begin_panic_handler
                               at /rustc/4d91de4e48198da2e33413efdcd9cd2cc0c46688/library\std\src\panicking.rs:692
  14:     0x7fffeee147d5 - core::panicking::panic_nounwind_fmt
                               at /rustc/4d91de4e48198da2e33413efdcd9cd2cc0c46688/library\core\src\intrinsics\mod.rs:3869
  15:     0x7fffeee14883 - core::panicking::panic_nounwind
                               at /rustc/4d91de4e48198da2e33413efdcd9cd2cc0c46688/library\core\src\panicking.rs:218
  16:     0x7fffeee14a57 - core::panicking::panic_cannot_unwind
                               at /rustc/4d91de4e48198da2e33413efdcd9cd2cc0c46688/library\core\src\panicking.rs:307
  17:     0x7ff832f93323 - library::my_destructor::__dtor
                               at C:\Users\user\.cargo\registry\src\index.crates.io-1949cf8c6b5b557f\ctor-0.4.0\src\macros\mod.rs:454
  18:     0x7ff82e3ef540 - _CxxFrameHandler3
  19:     0x7ff82e3e33d8 - is_exception_typeof
  20:     0x7ff839ed1c26 - RtlCaptureContext2
  21:     0x7ff832f93307 - library::my_destructor::__dtor
                               at C:\Users\user\.cargo\registry\src\index.crates.io-1949cf8c6b5b557f\ctor-0.4.0\src\macros\mod.rs:456
  22:     0x7ff8377142d6 - execute_onexit_table
  23:     0x7ff8377141fb - execute_onexit_table
  24:     0x7ff8377141b4 - execute_onexit_table
  25:     0x7ff832f989b9 - dllmain_crt_process_detach
                               at d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\dll_dllmain.cpp:180
  26:     0x7ff832f98ade - dllmain_dispatch
                               at d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\dll_dllmain.cpp:293
  27:     0x7ff839e49a1d - RtlActivateActivationContextUnsafeFast
  28:     0x7ff839e8f1ca - LdrShutdownProcess
  29:     0x7ff839e8ef7d - RtlExitUserProcess
  30:     0x7ff83842e3eb - FatalExit
  31:     0x7ff8377205bc - exit
  32:     0x7ff83772045f - exit
  33:     0x7ff73f6c13c3 - __scrt_common_main_seh
                               at d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:295
  34:     0x7ff838427374 - BaseThreadInitThunk
  35:     0x7ff839e7cc91 - RtlUserThreadStart
thread caused non-unwinding panic. aborting.
*/
// The Rust runtime sets the `packet` value of a thread before it exits: https://stdrs.dev/nightly/x86_64-pc-windows-gnu/std/thread/struct.JoinInner.html
// Since this value is never set in the case of thread termination, we get a "`Option::unwrap()` on a `None` value" panic.
// Rust has recently moved to showing a clearer error message that accounts for if a thread was terminated when joining a thread (this change has not made it into release builds yet): https://github.com/rust-lang/rust/issues/124466
// Terminating a thread is always an unsafe action, but that does not Windows from doing it behind our back.
// Rust runtime thread joining code: https://github.com/rust-lang/rust/blob/75530e9f72a1990ed2305e16fd51d02f47048f12/library/std/src/thread/mod.rs#L1739-L1753
