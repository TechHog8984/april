package tech.hog;

class Bruh {}
interface Meowing {
  public void meow();
}
class Dog implements Meowing {
  // ldc
  public String name = "unnamed";
  public void bark() {
    System.out.print(name);
    System.out.println(" says woof");
    // System.out.println(name + " says woof");
  }

  @Override
  public void meow() {
    System.out.print(name);
    System.out.println(" says meow");
  }
}

class Chihuahua extends Dog {
  // invokespecial
  Chihuahua(String namein) {
    // aload_0, aload_1, putfield
    name = namein;
  }
}

class a {
  // iconst_1
  private static int numfield = 1;
  // ldc
  private static int numfield2 = 1410421;
  // ldc2_w
  private static double numfield3 = 0.05;
  // dc
  private static float numfield4 = -14.0036f;
  // ldc2_w
  private static long numfield5 = 5213798342598754414L;
  public static void main(String[] args) {
    // invokevirtual, getstatic
    System.out.println("Hello, world!");
    // iconst_0
    System.out.println(0);
    // lconst_0
    System.out.println(0L);
    // lconst_1
    System.out.println(1L);
    System.out.println(numfield + ", " + numfield2 + ", " + numfield3 + ", " + numfield4 + ", " + numfield5);

    System.out.println(args.length);

    // new, dup
    Chihuahua chi = new Chihuahua("Dave");
    // astore_1, aload_1
    chi.bark();

    Meowing meowing_chi = (Meowing) chi;
    // invokeinterface
    meowing_chi.meow();

    System.out.println("is the thing a whatever?");
    // instanceof
    System.out.println((Object) chi instanceof Chihuahua);
    System.out.println((Object) chi instanceof Dog);
    System.out.println((Object) chi instanceof a);

    // iconst_2, iconst_3, iconst_4, iconst_5, invokestatic
    int inttest1 = integertest0(2, 3, 4, 5);
    System.out.println(inttest1);

    // ld2c_w, invokestatic
    System.out.println(longtest(41829152, 41829153, 41829154, 41829155));

    // sipush, invokestatic
    System.out.println(longtest2(29152, 493, 4895));

    long longvalue = 15351L;
    System.out.println(longvalue);

    longtest3();
    longtest4();
    longtest5();

    // pop
    integertest0(2, 10, 4, 4);
    // pop2
    longtest(48, 4953, 129, 4155);

    System.out.println("long arithmetic:" + longtest7() + ", " + longtest8() + ", " + longtest9() + ", " + longtest10());
    System.out.println("more: " + longtest11() + ", " + longtest12() + ", " + longtest13() + ", " + longtest14() + ", " + longtest15());
    System.out.println("neg: " + (-longtest7()) + ", " + (-longtest14()) + ", " + (-longtest9()));

    integertest2();

    // l2i
    System.out.println("aa: " + integertest1((int) longtest8()));

    // ldc, i2f, fconst_1
    float f1 = floattest(24124.f, 324.f, -0.323f, inttest1, 1.f);
    // fconst_0, fconst_1, fconst_2, ldc
    float f2 = floattest(0.f, 1.f, 2.f, 3.f, 4.f);
    System.out.println("floats: " + f1 + ", " + f2);

    floattest2();

    // fmul
    System.out.println(f1 * f2);
    // f2i, imul
    System.out.println((int)f1 * inttest1);

    // ldc2_w, dconst_0, dconst_1, i2d, dstore
    double d1 = doubletest(1352352.0, 0.0, 1.0, 2.0, inttest1);
    // dload, d2i
    double d2 = doubletest2((int) d1, d1, d1, (int) d1, 0.0);

    System.out.println("doubles: " + d1 + ", " + d2);

    doubletest3();
    doubletest4();

    // dload, d2i
    System.out.println((int)d1 * inttest1);

    // idiv
    System.out.println("aaaa: " + (inttest1 / (inttest1 + 2)));

    // astore
    Object out = System.out;
    // aload, if_acmpne, iconst_1, iconst_0, goto, istore
    boolean outeq = out == out;
    System.out.println("object stuff: " + objecttest1(System.out, System.out) + ", " + objecttest1(System.out, System.err) + ", " + outeq);

    System.out.println("more: " + objecttest2(System.out) + ", " + objecttest3());

    // aconst_null, astore, aload
    Object n = null;
    // ifnonnull
    if (n == null) {
      System.out.println("n was null lol");
    } else {
      System.out.println("n was not null wtf");
    }

    // aload, ifnull
    if (out != null) {
      System.out.println("out was not null");
    }

    int int1 = 0;
    // if_icmpge, iinc
    for (int i = 0; i < 4; i++) {
      int1 += 3;
    }
    System.out.println(int1);

    System.out.println("some bitwise:");
    // ishl, ishr, iushr, iand, ior, ixor
    System.out.println((int1 << 1) + ", " + (-int1 >> 1) + ", " + (-int1 >>> 1) + ", " + (int1 & 432) + ", " + (int1 & int1) + ", " + (int1 | 432) + ", " + (int1 ^ 432));

    System.out.println("some math:");
    // iconst_m1, imul
    System.out.println((-1) * int1);

    // if_icmpge
    if (int1 < 1) {
      System.out.println("int1 was less than 1");
    // if_icmpgt
    } else if (int1 <= 1) {
      System.out.println("int1 was less than or equal to 1");
    // if_icmple
    } else if (int1 > 1) {
      System.out.println("int1 was greater than 1");
    // if_icmplt
    } else if (int1 >= 1) {
      System.out.println("int1 was greater than or equal to 1");
    }
    // if_icmpne
    if (int1 == 1) {
      System.out.println("int1 was equal to 1");
    }
    // if_icmpeq
    if (int1 != 1) {
      System.out.println("int1 was not equal to 1");
    }

    System.out.println("bool array:");
    // newarray, iconst_0, iconst_1, iconst_2, iconst_3, iconst_1, i2b, bastore
    boolean barray[] = { false, true, false, (byte)(int1 - 12) == 0 ? false : true };
    // baload
    System.out.println(barray[0]);
    // arraylength, baload
    System.out.println(barray[barray.length - 1]);

    System.out.println("int array:");
    // newarray, iastore
    int iarray[] = { 1, -4, 59 };
    // arraylength, iaload
    for (int i = 0; i < iarray.length; i++)
      System.out.println(iarray[i]);

    System.out.println("char array:");
    // newarray, sipush, bipush, ldc, castore
    char carray[] = { 300, (char)-150, 99, (char)(int1 + 102) };
    // arraylength, caload
    for (int i = 0; i < carray.length; i++)
      System.out.println(carray[i]);

    System.out.println("multi-dimensional array:");
    // anewarray, aastore
    char doublecarray[][] = { carray, carray  };
    // arraylength, aaload
    for (int i = 0; i < doublecarray.length; i++)
      System.out.println(new String(doublecarray[i]));

    System.out.println("ldc with Class:");
    System.out.println(a.class);

    System.out.println("tableswitch:");
    System.out.println(tableswitch(0));
    System.out.println(tableswitch(1));
    System.out.println(tableswitch(2));
    System.out.println(tableswitch(3));
    System.out.println(tableswitch(4));

    System.out.println("lookupswitch:");
    System.out.println(lookupswitch(0));
    System.out.println(lookupswitch(1));
    System.out.println(lookupswitch(100));
    System.out.println(lookupswitch(200));
    System.out.println(lookupswitch(1000));

    System.out.println("long comparison:");
    // dmul, d2l, lcmp, ifeq, goto
    System.out.println(nativeTest2() != 5L);
    // ifne
    System.out.println(nativeTest2() == 5L);
    // iflt
    System.out.println(nativeTest2() >= 5L);
    // ifge
    System.out.println(nativeTest2() < 5L);
    // ifgt
    System.out.println(nativeTest2() <= 5L);
    // ifle
    System.out.println(nativeTest2() > 5L);

    // invokestatic (nativeTest)
    System.out.println(nativeTest());
  }

  private static native double nativeTest();
  private static native long nativeTest2();

  private static int integertest0(int a, int b, int c, int d) {
    // iload_0, iload_1, iload_2, iload_3
    // iadd, ireturn
    return a + b + c + d;
  }
  private static int integertest1(int a) {
    return a - 104;
  }
  private static void integertest2() {
    // sipush, istore_0
    int value1 = 210;
    // sipush, istore_1
    int value2 = 220;
    // sipush, istore_2
    int value3 = 230;
    // sipush, istore_3
    int value4 = 240;
    // sipush, istore
    int value5 = 250;
    // iadd, isub, iload_0, iload_1, iload_2, iload_3, iload
    System.out.println(value1 + value2 - value3 + value4 + value5);
  }
  private static float floattest(float a, float b, float c, float d, float e) {
    // fload_0, fload_1, fload_2, fload_3, fload, fadd, fsub, freturn
    return a + b - c + d + e;
  }
  private static void floattest2() {
    // ldc, fstore_0
    float value1 = 210.f;
    // ldc, fstore_1
    float value2 = 220.f;
    // ldc, fstore_2
    float value3 = 230.f;
    // ldc, fstore_3
    float value4 = 240.f;
    // ldc, fstore
    float value5 = 250.f;
    // fadd, fsub, fload_0, fload_1, fload_2, fload_3, fload
    System.out.println(value1 + value2 - value3 + value4 + value5);
  }
  private static double doubletest(double a, double b, double c, double d, double e) {
    // dload_0, dload_2, dload, dadd, dsub, dreturn
    return a + b - c + d + e;
  }
  private static double doubletest2(int a, double b, double c, int d, double e) {
    // iload_0, dload_1, dload_3, iload, i2d, dadd, dsub
    return a + b - c + d + e;
  }
  private static void doubletest3() {
    // ldc2_w, dstore_0
    double value1 = 210.d;
    // ldc2_w, dstore
    double value2 = 220.d;
    // ldc2_w, dstore_2
    double value3 = 230.d;
    // ldc2_w, dstore
    double value4 = 240.d;
    // ldc2_w, dstore
    double value5 = 250.d;
    // dadd, dsub, dload_0, dload_2, dload
    System.out.println(value1 + value2 - value3 + value4 + value5);
  }
  private static void doubletest4() {
    int value1 = 4;
    // ldc2_w, dstore_1
    double value2 = 220.d;
    // ldc2_w, dstore_3
    double value3 = 240.d;
    // ldc2_w, dstore
    double value4 = 250.d;
    // dadd, dsub, dload_1, dload_3, dload
    System.out.println(value1 + value2 - value3 + value4);
  }
  private static long longtest(long a, long b, long c, long d) {
    // lload_0, lload_2, lload
    // ladd, lreturn
    return a + b + c + d;
  }
  private static long longtest2(int a, long b, long c) {
    // iload_0, i2l, lload_1, lload_3
    // ladd, lreturn
    return a + b + c;
  }
  private static void longtest3() {
    // ldc2_w, lstore_0
    long value1 = 100L;
    // ldc2_w, lstore_2
    long value2 = 200L;
    // ldc2_w, lstore
    long value3 = 300L;
    // lsub, lload_0, lload_2, lload
    System.out.println(value1 - value2 - value3);
  }
  private static void longtest4() {
    // bipush, istore_0
    int value1 = 110;
    // ldc2_w, lstore_1
    long value2 = 120L;
    // sipush, istore_3
    int value3 = 130;
    // ldc2_w, lstore
    long value4 = 140L;
    // lsub, iload_0, lload_1, iload_3, lload, i2l
    System.out.println(value1 - value2 - value3 - value4);
  }
  private static void longtest5() {
    // bipush, istore_0
    int value1 = 110;
    // sipush, istore_1
    int value2 = 210;
    // sipush, istore_2
    int value3 = 310;
    // ldc2_w lstore_3
    long value4 = 410L;
    // iadd, lsub, iload_0, iload_1, iload_2, i2l, lload_3
    System.out.println(value1 + value2 + value3 - value4);
  }
  private static long longtest6() {
    // ldc2_w, lreturn
    return 1435153151351L;
  }
  private static long longtest7() {
    // ldc2_w, ladd, lreturn
    return longtest6() + 4153215L;
  }
  private static long longtest8() {
    // ldc2_w, lsub, lreturn
    return longtest6() - 44234243153215L;
  }
  private static long longtest9() {
    // ldc2_w, lmul, lreturn
    return longtest6() * 4153215L;
  }
  private static long longtest10() {
    // ldc2_w, ldiv, lreturn
    return longtest6() / 4153215L;
  }
  private static long longtest11() {
    // ldc, lshl, lreturn
    return longtest6() << 4153215L;
  }
  private static long longtest12() {
    // iconst_4, lshr, lreturn
    return longtest6() >> 4L;
  }
  private static long longtest13() {
    // iconst_5, lushr, lreturn
    return longtest6() >>> 5L;
  }
  private static long longtest14() {
    // ldc2_w, land, lreturn
    return longtest6() & 4134L;
  }
  private static long longtest15() {
    // ldc2_w, lor, lreturn
    return longtest6() | 41341L;
  }

  private static boolean objecttest1(Object a, Object b) {
    return a == b && a != b;
  }
  private static Object objecttest2(Object a) {
    return a;
  }
  private static Object objecttest3() {
    // getstatic, astore_0
    Object a = System.out;
    // getstatic astore_1
    Object b = System.out;
    // getstatic, astore_2
    Object c = System.err;
    // aload_1, astore_3
    Object d = b;
    // if_acmpne, aload_0, aload_1, aload_2, aload_3, astore, aload, areturn
    Object e = a == b ? c : d;
    return e;
  }

  private static boolean tableswitch(int a) {
    switch (a) {
      case 0:
      case 1:
      case 2:
        return true;
      default:
        return false;
    }
  }
  private static int lookupswitch(int a) {
    int result = 0;
    switch (a) {
      case 0:
        result += 10;
      case 100:
        result += 100;
      case 200:
        result += 1000;
        break;
      default:
        result -= 10;
    }

    return result;
  }
}
