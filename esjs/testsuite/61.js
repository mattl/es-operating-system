stdin = System.input;
stdout = System.output;
root = System.root;

iter = root.list("");
while (iter.hasNext())
{
    file = IFile(iter.next());
    name = file.name;
    stdout.write(name + '\n', name.length + 1);
}
"Ok";
