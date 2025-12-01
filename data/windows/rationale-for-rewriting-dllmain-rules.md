# Rationale for Rewriting `DllMain` Rules

The Microsoft Corporation's 2006 *Best Practices for Creating DLLs* whitepaper laid out some actions that cannot reliably be safely be done from `DllMain` or the module initializer and finalizer routines due to limitations of the Windows platform. These bad actions are provided as opaque "rules" (as some refer to them as) that one must not break with no explanation as to why or the root cause. The intention for rewriting the rules for `DllMain` is to turn the tables on the operating system: to find out what went wrong to make module initializers and finalizers so hopelessly broken on Windows.

Be mindful of the fact that my critiques apply only to the software or company itself and not to the people who contribute to its development. I appreciate and respect the people who put hard work into creating the technologies. Thank you!

[See here for the 2006 Microsoft whitepaper.](dll-best-practices/README.md)

[See here for early writings on module initialization and finalization on Windows preceding Microsoft's official whitepaper.](module-constructor-and-destructor-writings-timeline.md)
