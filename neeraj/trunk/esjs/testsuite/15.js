function x()
{
    throw 5;
}

a = 0;
try
{
    x();
}
catch (e)
{
    a = e;
}
finally
{
    a = 5 * a;
}
a;
