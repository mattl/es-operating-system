a = 3;
x = 4;
delete a;
try
{
    a;
}
catch (e)
{
    x = e;
}
x;
