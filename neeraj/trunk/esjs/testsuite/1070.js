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

s = "HELLO WORLD!";
l = s;
check(s.toLowerCase() == l.toLocaleLowerCase());
check(s.toLowerCase() == l.toLocaleLowerCase());

s = "HeLlO wOrLd!";
l = s;
check(s.toLowerCase() == l.toLocaleLowerCase());

s = "ＡＢＣＤＥＦＧ";
l = s;
check(s.toLowerCase() == l.toLocaleLowerCase());

s = "こんにちは世界";
l = s;
check(s.toLowerCase() == l.toLocaleLowerCase());

