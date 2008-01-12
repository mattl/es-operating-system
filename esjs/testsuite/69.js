stream = System.output;
r = new RegExp("^ab", "g");
for (;;)
{
    a = r.exec("ababab");
    if (!a)
    {
        break;
    }
    for (var i = 0; i < a.length; ++i)
    {
        stream.write(String(a[i]), String(a[i]).length);
        stream.write("\n", 1);
    }
}
"Ok.";
