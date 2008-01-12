// check get[UTC]XXX()
stdout = System.output;
function print(s)
{
    stdout.write(s, s.length);
    stdout.write("\n", 1);
    return 0;
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

function printUTC(d)
{
    stdout.write('UTC   : ', 8);
    s = String(d.getUTCFullYear());
    stdout.write(s, s.length);
    stdout.write('/', 1);

    s = String(d.getUTCMonth()+1);
    stdout.write(s, s.length);
    stdout.write('/', 1);

    s = String(d.getUTCDate());
    stdout.write(s, s.length);
    stdout.write(' ', 1);

    s = String(d.getUTCHours());
    stdout.write(s, s.length);
    stdout.write(':', 1);

    s = String(d.getUTCMinutes());
    stdout.write(s, s.length);
    stdout.write(':', 1);

    s = String(d.getUTCSeconds());
    stdout.write(s, s.length);
    stdout.write('.', 1);

    s = String(d.getUTCMilliseconds());
    stdout.write(s, s.length);
    stdout.write(' (', 2);

    s = String(d.getUTCDay());
    stdout.write(s, s.length);
    stdout.write(')\n ', 2);

    return 0;
}

function printLocalTime(d)
{
    stdout.write('Local : ', 8);
    s = String(d.getFullYear());
    stdout.write(s, s.length);
    stdout.write('/', 1);

    s = String(d.getMonth()+1);
    stdout.write(s, s.length);
    stdout.write('/', 1);

    s = String(d.getDate());
    stdout.write(s, s.length);
    stdout.write(' ', 1);

    s = String(d.getHours());
    stdout.write(s, s.length);
    stdout.write(':', 1);

    s = String(d.getMinutes());
    stdout.write(s, s.length);
    stdout.write(':', 1);

    s = String(d.getSeconds());
    stdout.write(s, s.length);
    stdout.write('.', 1);

    s = String(d.getMilliseconds());
    stdout.write(s, s.length);
    stdout.write(' (', 2);

    s = String(d.getDay()); // 0 represents Sunday.
    stdout.write(s, s.length);
    stdout.write(')\n ', 2);

    return 0;
}

d = new Date(2007, 7-1, 18, 13, 53, 7, 865);

s = d.toString();
stdout.write("(local) ", 8);
stdout.write(s, s.length);
stdout.write('\n', 1);

printUTC(d);
printLocalTime(d);

check(d.getFullYear() == 2007);
check(d.getMonth() == 6);
check(d.getDate() == 18);
check(d.getHours() == 13);
check(d.getMinutes() == 53);
check(d.getSeconds() == 7);
check(d.getMilliseconds() == 865);
check(d.getDay() == 3);

check(d.getHours() - d.getUTCHours() == 9);
check(d.getFullYear() == d.getUTCFullYear());
check(d.getMonth() == d.getUTCMonth());
check(d.getDate() == d.getUTCDate());
check(d.getMinutes() == d.getUTCMinutes());
check(d.getSeconds() == d.getUTCSeconds());
check(d.getMilliseconds() == d.getUTCMilliseconds());
check(d.getDay() == d.getUTCDay());

check(d.getMonth() == 7-1);
