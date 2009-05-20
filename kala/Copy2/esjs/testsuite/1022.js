// test setMonth().

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

d = new Date(1999, 1-1, 1, 0, 0, 1 /* localtime */);
s = d.toLocaleString(); // LocalTime
stdout.write(s, s.length);
stdout.write("\n", 1);

d.setMonth(12-1);
s = d.toLocaleString();
check(s == "Wed Dec  1 00:00:01 1999");

d.setMonth(12-1, 30);
s = d.toLocaleString();
check(s == "Thu Dec 30 00:00:01 1999");

// the 3rd. argumet is ignored.
d.setMonth(12-1, 30, 23);
s = d.toLocaleString();
check(s == "Thu Dec 30 00:00:01 1999");
