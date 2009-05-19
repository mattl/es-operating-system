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

n = new Number(123.45678901);
m = n.valueOf();
check(m == 123.45678901);

check(n.toLocaleString() == n.toString());
