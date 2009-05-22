// check Date(string)
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

s = "Wed Jul 11 11:53:01 2007";
d = new Date(s /* localtime */);
check(String(d) == s);
