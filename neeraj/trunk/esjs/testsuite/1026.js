// test setHours().

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

d = new Date(1900, 4-1, 2, 17, 1, 30 /* localtime */);
s = d.toString(); // LocalTime
stdout.write(s + '\n', s.length + 1);

d.setHours(23);
s = d.toLocaleString();

check(s == "Mon Apr  2 23:01:30 1900");

d.setHours(23, 10);
s = d.toLocaleString();

check(s == "Mon Apr  2 23:10:30 1900");

d.setHours(23, 10, 20);
s = d.toLocaleString();

check(s == "Mon Apr  2 23:10:20 1900");

d.setHours(23, 10, 20, 123);
s = d.toLocaleString();

check(s == "Mon Apr  2 23:10:20 1900");
