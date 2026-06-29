package tech.hog;

class Dog {
  public String name = "unnamed";
  public void bark() {
    System.out.print(name);
    System.out.println(" says woof");
    // System.out.println(name + " says woof");
  }
}

class Chihuahua extends Dog {
  Chihuahua(String namein) {
    name = namein;
  }
}

class a {
  private static int numfield = 1;
  private static int numfield2 = 1410421;
  private static double numfield3 = 0.05;
  private static float numfield4 = -14.0036f;
  private static long numfield5 = 5213798342598754414L;
  public static void main(String[] args) {
    System.out.println("Hello, world!");
    System.out.println(numfield);
    System.out.println(numfield2);
    System.out.println(numfield3);
    System.out.println(numfield4);
    System.out.println(numfield5);

    Chihuahua a = new Chihuahua("Dave");
    a.bark();
  }
}
