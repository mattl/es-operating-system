var stdout = System.getOut();

function print(stream)
{
    while ((c = stream.read(255)) != '')
    {
        stdout.write(c, c.length);
    }
}

for (var i = 1; i < params.length; ++i)
{
    try
    {
        var file = IFile(root.lookup(params[i]));
        var stream = file.getStream();
        print(stream);
    }
    finally
    {
    }
}
