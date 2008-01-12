stdin = System.getIn();
stdout = System.getOut();
root = System.getRoot();

iter = root.list("");
while (iter.hasNext())
{
    file = IFile(iter.next());
    name = file.getName(256);
    stdout.write(name + '\n', name.length + 1);
}
"Ok";
