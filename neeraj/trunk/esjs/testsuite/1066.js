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

s = "Hello World!";
check(s.lastIndexOf("lo", 12) == 3);
check(s.lastIndexOf("lo", 3) == 3);
check(s.lastIndexOf("lo", 2) == -1);
check(s.lastIndexOf("!") == 11);
check(s.lastIndexOf("lo", -1) == -1);
check(s.lastIndexOf("lo", 100) == 3);
check(s.lastIndexOf("", 100) == 12);
check(s.lastIndexOf("", -100) == 0); // [check] should return -1?
