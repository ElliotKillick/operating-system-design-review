// WORK IN PROGRESS

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
// 4. Two-phase initialization is an anti-pattern
//   - The same can reasonably extend to lazy initialization, although lazy initialization trades a partial initialization danger for a perfomance hit by constantly checking for initialization on every use
// 5. Operating systems should be sane
//   - It is not like any operating system would be broken and unhinged enough to go around terminating threads, right? Right?

pub fn hello() {
    println!("Hello from the Rust dynamic library!");
}
