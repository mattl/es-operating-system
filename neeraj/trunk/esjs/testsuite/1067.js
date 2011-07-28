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
check(s.localeCompare(s) == 0);
check(s.localeCompare("Haaaa") > 0);
check(s.localeCompare("Hello Wor") > 0);
check(s.localeCompare("Hello") > 0);
check(s.localeCompare("H") > 0);
check(s.localeCompare("World") < 0);
check(s.localeCompare("Hell World") > 0);
check(s.localeCompare("World!") < 0);
check(s.localeCompare("Hello World!!") < 0);
check(s.localeCompare("こんにちは") < 0);
check(s.localeCompare("") > 0);
check(s.localeCompare() < 0);
