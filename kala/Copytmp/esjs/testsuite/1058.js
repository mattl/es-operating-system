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

a = new Array("h", "ab", "ef", "g", "cd", "i");
s0 = a[0];
s1 = a[1];
s2 = a[2];
s3 = a[3];
s4 = a[4];
s5 = a[5];

check(s0 == a.shift());
check(s1 == a.shift());
check(s2 == a.shift());
check(s3 == a.shift());
check(s4 == a.shift());
check(s5 == a.shift());

