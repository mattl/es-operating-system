for (var i = 1; i < params.length; ++i)
{
    try
    {
        cwd.unbind(params[i]);
    }
    catch (e)
    {
    }
}
