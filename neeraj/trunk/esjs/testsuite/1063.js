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
// check(s == "Hello");

check(s.charCodeAt(0) == 0x0048);
check(s.charCodeAt(1) == 0x0065);
check(s.charCodeAt(2) == 0x006C);
check(s.charCodeAt(3) == 0x006C);
check(s.charCodeAt(4) == 0x006F);

s = String.fromCharCode(0x3053, 0x3093, 0x306B, 0x3061, 0x306F, 0x4E16, 0x754C);
// check(s == "こんにちは世界");
check(s.charCodeAt(0) == 0x3053);
check(s.charCodeAt(3) == 0x3093);
check(s.charCodeAt(6) == 0x306B);
check(s.charCodeAt(9) == 0x3061);
check(s.charCodeAt(12) == 0x306F);
check(s.charCodeAt(15) == 0x4E16);
check(s.charCodeAt(18) == 0x754C);

s = String.fromCharCode();
// check(s == "");
check(isNaN(s.charCodeAt(0)));
