var stdout = System.getOut();
var root = System.getRoot();

if (params.length < 2)
{
    params[1] = "";
}

for (var i = 1; i < params.length; ++i)
{
    try
    {
        var iter = root.list(params[i]);
        while (iter.hasNext())
        {
            unknown = iter.next();
            file = IFile(unknown);
            name = file.getName(256);
            if (name != "." && name != "..")
            {
                stdout.write(name + '\n', name.length + 1);
            }
            file.release();
            unknown.release();
        }
        iter.release();
    }
    finally
    {
    }
}

stdout.release();
root.release();
