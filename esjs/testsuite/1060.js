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

t = new Array();
t[0] = new Array(1, 2, 3);
t[1] = new Array(10, 11, 12);

check(t[0][0] == 1);
check(t[0][1] == 2);
check(t[0][2] == 3);
check(t[1][0] == 10);
check(t[1][1] == 11);
check(t[1][2] == 12);


delete t[1][2];
check(t[1][2] == undefined);
delete t[0];
t.join(" ");
check(t[0] == undefined);
check(t[1][1] == 11);
