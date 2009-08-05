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
check(s.toLowerCase() == "hello world!");
check(s.toLowerCase() == "hello world!");

s = "HeLlO wOrLd!";
check(s.toLowerCase() == "hello world!");

s = "ＡＢＣＤＥＦＧ";
check(s.toLowerCase() == "ａｂｃｄｅｆｇ");

s = "こんにちは世界";
check(s.toLowerCase() == "こんにちは世界");

s = "ΑΒ";
check(s.toLowerCase() == "αβ");
