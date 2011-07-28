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

n = new Number(0.000001234567);
s = n.toPrecision(2);
check(s == "0.0000012");

n = new Number(0.0000001234567);
s = n.toPrecision(2);
check(s == "1.2e-07");

n = new Number(1234.567);
s = n.toPrecision(2);
check(s == "1.2e+03");

n = new Number(123.4567);
s = n.toPrecision(2);
check(s == "1.2e+02");

n = new Number(12.34567);
s = n.toPrecision(2);
check(s == "12");

n = new Number(1.234567);
s = n.toPrecision(1);
check(s == "1");

n = new Number(1234567.89012345);
s = n.toPrecision();
check(s == "1234567.89012345");
