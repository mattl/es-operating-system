// check date before 1900(?)
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

d = new Date(1234, 5-1, 6, 7, 8, 9, 0);
String(d);

check(d.getFullYear() == 1234);
check(d.getMonth() == 5-1);
check(d.getDate() == 6);
check(d.getHours() == 7);
check(d.getMinutes() == 8);
check(d.getSeconds() == 9);
check(d.getMilliseconds() == 0);
