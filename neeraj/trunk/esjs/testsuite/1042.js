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

n = new Number(0.01234567);

s = n.toPrecision(2);
check(s == "0.012");

s = n.toPrecision(4);
check(s == "0.01235");

s = n.toPrecision(15);
check(s == "0.0123456700000000");

n = new Number(-1.234567);
n.toPrecision(3);
check(n.toPrecision(3) == "-1.23");

n = new Number(1234567);
check(n.toPrecision(4) == "1.235e+06");
n.toPrecision(4);

n.toPrecision(20);
check(n.toPrecision(20) == "1234567.0000000000000");

n = new Number(0.01234567);
try
{
    n.toPrecision(-1);
}
catch (e)
{
    a = e;
}
check(a.name == "RangeError");

try
{
    n.toPrecision(21);
}
catch (e)
{
    a = e;
}
check(a.name == "RangeError");

try
{
    n.toPrecision(NaN);
}
catch (e)
{
    b = e;
}
check(a.name == "RangeError");

n = new Number(Infinity);
check(n.toPrecision(2) == "Infinity");

n = new Number(0);
check(n.toPrecision(1) == "0");

n = new Number(0);
check(n.toPrecision(8) == "0.0000000");

n = new Number(1234567);
check(n.toPrecision(3) == "1.23e+06");
