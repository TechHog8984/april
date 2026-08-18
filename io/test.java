import java.security.AccessController;
import java.security.PrivilegedAction;

class test {
  public static void main(String[] args) {
    PrivilegedAction<Integer> action = () -> staticmethod();

    String a = args.toString();

    int num = AccessController.doPrivileged(action);
    System.out.println(num);
  }

  private static int staticmethod() {
    return 42;
  }
}
