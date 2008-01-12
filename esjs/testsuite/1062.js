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

s = String.fromCharCode(0x0048, 0x0065, 0x006C, 0x006C, 0x006F);
check(s == "Hello");

s = String.fromCharCode(0x3053, 0x3093, 0x306B, 0x3061, 0x306F, 0x4E16, 0x754C);
check(s == "こんにちは世界");

s = String.fromCharCode();
check(s == "");
