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
s = n.toExponential(2);
check(s == "1.23e-02");

s = n.toExponential(4);
check(s == "1.2346e-02");

s = n.toExponential(10);
check(s == "1.2345670000e-02");

n = new Number(-1.234567);
check(n.toExponential(3) == -1.235e+00);

n = new Number(1234567);
check(n.toExponential(4) == 1.2346e+06);

check(n.toExponential(20) == 1.23456700000000000000e+06);

n = new Number(0.01234567);
try
{
    n.toExponential(-1);
}
catch (e)
{
    a = e;
}
check(a.name == "RangeError");

try
{
    n.toExponential(21);
}
catch (e)
{
    a = e;
}
check(a.name == "RangeError");

try
{
    n.toExponential(NaN);
}
catch (e)
{
    b = e;
}
check(a.name == "RangeError");

n = new Number(Infinity);
check(n.toExponential(2) == "Infinity");

n = new Number(0);
n.toExponential();
check(n.toExponential() == "0e+00");

n = new Number(1234567);
check(n.toExponential() == "1.234567e+06");

