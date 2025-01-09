.global _start             // Provide program starting address to linker
.p2align 3 // Feedback from Peter
.extern printf

// Setup the parameters to print hello world
// and then call Linux to do it.
.section __TEXT,__text,regular,pure_instructions

_start: mov X0, #1     // 1 = StdOut
adr X1, helloworld // string to print
mov X2, #13     // length of our string
mov X16, #4     // MacOS write system call
svc 0     // Call linux to output the string

; Setup the parameters to exit the program
// and then call Linux to do it.

mov     X0, #0      // Use 0 return code
mov     X16, #1     // Service command code 1 terminates this program
svc     0           // Call MacOS to terminate the program

helloworld:      .ascii  "Hello World!\n"
