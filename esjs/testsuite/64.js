stdin = System.getIn();
stdout = System.getOut();
root = System.getRoot();

filename = "Makefile";

function print(stream)
{
    while ((c = stream.read(255)) != '')
    {
        stdout.write(c, c.length);
    }
}

file = IFile(root.lookup(filename));
stream = file.getStream();
print(stream);
"Ok";
