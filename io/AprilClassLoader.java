import java.lang.ClassLoader;
import java.net.URL;

public class AprilClassLoader extends ClassLoader {
  @Override
  protected URL findResource(String name) {
    try {
      String newname;
      // System.out.println("name: " + name + ", startsWithslash: " + name.startsWith("/"));
      if (name.startsWith("/")) {
        newname = "file:///home/user/Documents/resourcestest/" + name;
      } else {
        newname = "file:///home/user/Documents/resourcestest/" + getParentPath() + name;
      }
      // System.out.println("I AM PRINTING THE THING HERE YOU GO: " + newname);
      return new URL(newname);
    } catch (Exception e) {
      return null;
    }
  }

  private native String getParentPath();
}
