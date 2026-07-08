# april

april is a Java bytecode to Lua source code conversion tool.

# NOTES
1. april is WORK IN PROGRESS and UNSTABLE!
2. april targets Java 8
3. april outputs [Luau](https://github.com/luau-lang/luau) with potential plans for supporting other dialects in the future 

# BUILDING
```bash
git clone https://github.com/TechHog8984/april.git
cd april
cmake -B build -S . && cmake --build build
```

# RUNNING
```bash
./build/april path/to/classfile/foo.class --output=out/foo.luau
```

## ACTUALLY DOING SOMETHING
this section is in construction as I am overhauling things
