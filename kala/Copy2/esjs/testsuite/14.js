a = 0;
try
{
    throw 5;
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
