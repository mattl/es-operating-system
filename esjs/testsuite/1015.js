function print(s)
{
    stdout = System.getOut();
    stdout.write(s, s.length);
    stdout.write("\n", 1);
    return 0;
}

function check(result)
{
    stdout = System.getOut();
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

tick = 1184734387865;
d = new Date(tick);
print(d.toLocaleString());
time = d.getTime();
check(time == tick);
