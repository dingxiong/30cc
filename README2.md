# Compile 

```
as -o hello.o hello.s
ld -macos_version_min 15.0.0 -o hello hello.o -lSystem -syslibroot `xcrun -sdk macosx --show-sdk-path` -e _start -arch arm64
```
