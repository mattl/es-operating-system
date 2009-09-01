var n = true;
for (var i = 1; i < params.length; ++i)
{
    if (params[i] == "-n")
    {
        n = false;
        continue;
    }

    stdout.write(params[i] + ' ', params[i].length + 1);
}
if (n)
{
    stdout.write('\n', 1);
}
