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

d = new Date(2008, 7-1, 18, 22, 20, 59, 30 /* localtime */);
s = d.toLocaleTimeString();
check (s == "22:20:59");
