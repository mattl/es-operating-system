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
        var file = File(cwd.lookup(params[i]));
        var stream = file.stream;
        print(stream);
    }
    catch (e)
    {
    }
}
