// test setDate

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

d = new Date(1970, 1-1, 31, 15, 0, 1);
s = d.toLocaleString(); // LocalTime
stdout.write(s + '\n', s.length + 1);

d.setDate(15);
s = d.toLocaleString();
check(s == "Thu Jan 15 15:00:01 1970");

