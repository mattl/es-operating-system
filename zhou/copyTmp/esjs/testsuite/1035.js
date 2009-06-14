// error case.

stdout = System.output;
function check(result)
{
    if (result)
    {
        stdout.write("OK\n", 3);
    }
    else
    {
        stdout.write("*** ERROR ***\n", 14);
    }
    return result;
}

d = new Date(2007, 7-1, 17, 15, 40, 59);

try
{
    d.undefinedMethod(); // call an undefined method.
}
catch (e)
{
    a = e;
}
check(a.name == "TypeError");
