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

n = new Number(123456);
s = n.toString();
check(s == "123456");

n = new Number(123456.789012345);
s = n.toString();
check(s == "123456.789012345");

n = new Number(15);
check(n.toString(16) == "f");
check(n.toString(8) == "17");
check(n.toString(2) == "1111");

check(n.toString(32) == "f");
check(n.toString(36) == "f");

n = new Number(90);
check(n.toString(36) == "2i");
check(n.toString(32) == "2q");
check(n.toString(3) == "10100");

n = new Number(-90);
check(n.toString(36) == "-2i");
check(n.toString(32) == "-2q");
check(n.toString(3) == "-10100");

n = new Number(314.15);
m = new Number(314);
check(n.toString(36) == m.toString(36));
check(n.toString(16) == m.toString(16));
check(n.toString(8)  == m.toString(8));
check(n.toString(2)  == m.toString(2));

try
{
    n.toString(37);
}
catch (e)
{
    a = e;
}
check(a.name == "RangeError");

try
{
    n.toString(1);
}
catch (e)
{
    a = e;
}
check(a.name == "RangeError");
