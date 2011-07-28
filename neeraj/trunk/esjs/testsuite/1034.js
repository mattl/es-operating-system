// test getYear() and setYear().

stdout = System.output;
function print(s)
{
    stdout.write(s + '\n', s.length + 1);
}

function check(result)
{
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

d = new Date(1950, 1-1, 2, 3, 4, 56 /* localtime */);
print(d.toString());

check(d.getYear() == 50);

d.setYear(60);
check(d.getYear() == 60);

