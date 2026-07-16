package tech.hog;

class b {
  public static void main(String[] args) {
    System.out.println("Hello from b");
    System.out.println(b.class.getResourceAsStream("/someresource.txt"));
  }
}
