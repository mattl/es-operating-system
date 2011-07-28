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
check(s.indexOf("lo") == 3);
check(s.indexOf("lo", 0) == 3);
check(s.indexOf("lo", 3) == 3);
check(s.indexOf("lo", 4) == -1);
check(s.indexOf("lo", 10) == -1);
check(s.indexOf("lo", 100) == -1);
check(s.indexOf("lo", -100) == 3);
check(s.indexOf("", 3) == 3);

check(s.indexOf("", 20) == 12); // [check] should return -1?
