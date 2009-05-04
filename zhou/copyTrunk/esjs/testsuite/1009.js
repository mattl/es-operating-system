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

d = new Date(2009, 7-1, 18 /* localtime */);
s = d.toDateString();
check(s == "07/18/09");
