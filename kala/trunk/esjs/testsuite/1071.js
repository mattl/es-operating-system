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
check(s.toUpperCase() == "HELLO WORLD!");
check(s.toUpperCase() == "HELLO WORLD!");

s = "HeLlO wOrLd!";
check(s.toUpperCase() == "HELLO WORLD!");

s = "ａｂｃｄｅｆｇ";
check(s.toUpperCase() == "ＡＢＣＤＥＦＧ");

s = "こんにちは世界";
check(s.toUpperCase() == "こんにちは世界");

s = "αβ";
check(s.toUpperCase() == "ΑΒ");
