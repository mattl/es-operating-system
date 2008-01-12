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
try
{
    n.toFixed(-1);
}
catch (e)
{
    a = e;
}
check(a.name == "RangeError");

try
{
    n.toFixed(21);
}
catch (e)
{
    a = e;
}
check(a.name == "RangeError");

try
{
    n.toFixed(NaN);
}
catch (e)
{
    b = e;
}
check(b.name == "RangeError");

check(n.toFixed(1) == "0.0");
check(n.toFixed(2) == "0.01");

n.toFixed(6);
check(n.toFixed(6) == "0.012346");
check(n.toFixed(10) == "0.0123456700");
check(n.toFixed(18) == "0.012345670000000000");

// check(n.toFixed(20) == "0.01234567000000000000"); // the significant digit of double is 14 or 15...

check(n.toFixed(0) == "0");

n = new Number(-987.654);
check(n.toFixed(0) == "-988");


n = new Number(10.0);
check(n.toFixed(3) == "10.000");

n = new Number(1.0e21);
check(n.toFixed(1) == 1000000000000000000000.000000); // [check]
