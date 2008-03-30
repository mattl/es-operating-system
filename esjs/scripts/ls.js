if (params.length < 2)
{
    params[1] = "";
}

for (var i = 1; i < params.length; ++i)
{
    try
    {
        var iter = cwd.list(params[i]);
        while (iter.hasNext())
        {
            unknown = iter.next();
            binding = Binding(unknown);
            name = binding.name;
            if (name != "." && name != "..")
            {
                stdout.write(name + '\n', name.length + 1);
            }
            binding.release();
            unknown.release();
        }
        iter.release();
    }
    catch (e)
    {
    }
}
