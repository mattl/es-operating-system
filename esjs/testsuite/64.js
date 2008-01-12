stdin = System.input;
stdout = System.output;
root = System.root;

filename = "Makefile";

function print(stream)
{
    while ((c = stream.read(255)) != '')
    {
        stdout.write(c, c.length);
    }
}

file = IFile(root.lookup(filename));
stream = file.stream;
print(stream);
"Ok";
