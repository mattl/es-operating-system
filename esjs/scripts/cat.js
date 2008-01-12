function print(stream)
{
    var buf;

    while ((buf = stream.read(255)) != '')
    {
        stdout.write(buf, buf.length);
    }
}

for (var i = 1; i < params.length; ++i)
{
    try
    {
        var file = IFile(cwd.lookup(params[i]));
        var stream = file.getStream();
        print(stream);
    }
    catch (e)
    {
    }
}
