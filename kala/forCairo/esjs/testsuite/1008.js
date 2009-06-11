// check Date.UTC().
function check(result)
{
    stdout = System.output;
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

num = Date.UTC(2007, 7-1, 13, 11, 39, 30 /* UTC */);
d = new Date(num);
check(d.toUTCString() == "Fri Jul 13 11:39:30 2007 UTC");