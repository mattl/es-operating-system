// test toLocaleString().
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

d = new Date(2007, 7-1, 18, 13, 20, 59, 30 /* localtime */);
s = d.toLocaleString();
check(s == "Wed Jul 18 13:20:59 2007");
