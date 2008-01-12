if (params.length <= 1)
{
    cwd = root;
}
else
{
    cwd = IContext(cwd.lookup(params[1]));
}
