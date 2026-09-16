# A more efficient verison of sslExample (with no direct syscalls)

This is a variant of `../sslExample`, with the `access()` syscall removed. That syscall was used to test whether the page containing the candidate capability is mapped. Since this is a syscall, it triggers a transition to the kernel and back, so can be quite expensive if the range to be scanned is huge. To remove this overhead, we will simply deference a candidate pointer; if it's not mapped, it will trigger a page fault, which will be caught by our signal handler.

To compile, run `make test`. To run this with cheric18n/cherirevoke enabled/disabled, use proccontrol. For example, to run it with cheric18n enabled and cherirevoke disabled, use this command:

```
proccontrol -m cheric18n -s enable proccontrol -m cherirevoke -s disable ./bin/test
```

