stdin = System.input;
stdout = System.output;
root = System.root;

filename = "file/Makefile";

function print(stream)
{
    while ((c = stream.read(255)) != '')
    {
        stdout.write(c, c.length);
    }
}

file = File(root.lookup(filename));
stream = file.stream;
print(stream);
"Ok";
