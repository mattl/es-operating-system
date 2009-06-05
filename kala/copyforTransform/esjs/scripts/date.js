var utc = false;
for (var i = 1; i < params.length; ++i)
{
    if (params[i] == "-u")
    {
        utc = true;
    }
}

var d = new Date();
var s;
if (utc)
{
    s = d.toUTCString();
}
else
{
    s = d.toString();
}

stdout.write(s + '\n', s.length + 1);
