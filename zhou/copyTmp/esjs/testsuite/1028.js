// test setMinutes().

stdout = System.output;
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

d = new Date(1900, 4-1, 2, 8, 1, 30 /* localtime */);
s = d.toLocaleString(); // LocalTime
stdout.write(s + '\n', s.length + 1);

d.setMinutes(10);
s = d.toLocaleString();

check(s == "Mon Apr  2 08:10:30 1900");

d.setMinutes(10, 20);
s = d.toLocaleString();

check(s == "Mon Apr  2 08:10:20 1900");

d.setMinutes(10, 20, 123);
s = d.toLocaleString();

check(s == "Mon Apr  2 08:10:20 1900");
check(d.getMilliseconds() == 123);
