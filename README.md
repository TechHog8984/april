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
An output of april will not do anything on its own. For now, you can use the TEMPORARY [runner script](./io/runner.luau).

The runner script has a stub for System.out.println so basic programs like the following can run:
```java
package tech.hog;

class a {
  private static int numfield = 1;
  private static int numfield2 = 1410421;
  private static double numfield3 = 0.05;
  private static float numfield4 = -14.0036f;
  public static void main(String[] args) {
    System.out.println("Hello, world!");
    System.out.println(numfield);
    System.out.println(numfield2);
    System.out.println(numfield3);
    System.out.println(numfield4);
  }
}
```
```bash
./build/april ./io/bin/tech/hog/a.class -o=io/out.luau
lune run runner.luau ./out.luau
```
```
Hello, world!
1
1410421
0.05
-14.0036
```
