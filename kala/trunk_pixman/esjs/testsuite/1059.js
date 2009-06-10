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

check(a.length == 6);
a[10] = "hello";
check(a.length == 11);
a["first"] = "world";
check(a["first"] == "world");

delete a["first"];
check(a["first"] == undefined);

b = new Array(3);
for (i = 0; i < b.length; i++)
{
    b[i] = new Array(4);
    for (j = 0; j < b[i].length; j++)
    {
        b[i][j] = i * 10 + j;
    }
}
b[2][3];

