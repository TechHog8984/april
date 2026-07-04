package tech.hog;

class Bruh {}
class Dog {
  // ldc
  public String name = "unnamed";
  public void bark() {
    System.out.print(name);
    System.out.println(" says woof");
    // System.out.println(name + " says woof");
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
    System.out.println(numfield);
    System.out.println(numfield2);
    System.out.println(numfield3);
    System.out.println(numfield4);
    System.out.println(numfield5);

    System.out.println(args.length);

    // new, dup
    Chihuahua chi = new Chihuahua("Dave");
    // astore_1, aload_1
    chi.bark();

    System.out.println("is the thing a whatever?");
    // instanceof
    System.out.println((Object) chi instanceof Chihuahua);
    System.out.println((Object) chi instanceof Dog);
    System.out.println((Object) chi instanceof a);

    // iconst_2, iconst_3, iconst_4, iconst_5, invokestatic
    System.out.println(integertest(2, 3, 4, 5));

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
    integertest(2, 10, 4, 4);
    // pop2
    longtest(48, 4953, 129, 4155);

    integertest2();

    System.out.println("object stuff:");

    System.out.println(objecttest1(System.out, System.out));
    System.out.println(objecttest1(System.out, System.err));
    // astore
    Object out = System.out;
    // aload, if_acmpne, iconst_0, iconst_1, goto
    System.out.println(out == out);

    System.out.println(objecttest2(System.out));
    System.out.println(objecttest3());

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
    // ishl
    System.out.println(int1 << 1);
    // ishr
    System.out.println(-int1 >> 1);
    // iushr
    System.out.println(-int1 >>> 1);
    // iand
    System.out.println(int1 & 432);
    // iand
    System.out.println(int1 & int1);
    // ior
    System.out.println(int1 | 432);
    // ixor
    System.out.println(int1 ^ 432);

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

  private static int integertest(int a, int b, int c, int d) {
    // iload_0, iload_1, iload_2, iload_3
    // iadd, ireturn
    return a + b + c + d;
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
    // iadd, lsub, iload_0, iload-1, iload_2, i2l, lload_3
    System.out.println(value1 + value2 + value3 - value4);
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
}
