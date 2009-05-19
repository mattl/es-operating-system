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

n = new Number(123.456);
n.toString();
check(n.toString() == "123.456");
check(n.toLocaleString() == n.toString());

