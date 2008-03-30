if (params.length <= 1)
{
    cwd = root;
}
else
{
    cwd = Context(cwd.lookup(params[1]));
}
