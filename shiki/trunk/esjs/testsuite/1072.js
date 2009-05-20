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

s = "hello world!";
l = s;
check(s.toUpperCase() == l.toLocaleUpperCase());
check(s.toUpperCase() == l.toLocaleUpperCase());

s = "HeLlO wOrLd!";
l = s;
check(s.toUpperCase() == l.toLocaleUpperCase());

s = "ａｂｃｄｅｆｇ";
l = s;
check(s.toUpperCase() == l.toLocaleUpperCase());

s = "こんにちは世界";
l = s;
check(s.toUpperCase() == l.toLocaleUpperCase());

