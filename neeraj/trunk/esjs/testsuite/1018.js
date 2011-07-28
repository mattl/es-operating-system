// test setTime().

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

d0 = new Date(2007, 7-1, 17, 11, 4, 30, 0);
d = new Date();
d.setTime(d0);
s = d.toString();

check(s == "Tue Jul 17 11:04:30 2007");

invalid = new Array(1);

try
{
    d.setTime(invalid);
}
catch (e)
{
    a = e;
}
check(a.name == "TypeError");
