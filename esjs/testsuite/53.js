r = new RegExp("(z)((a+)?(b+)?(c))*");
a = r.exec("zaacbbbcac");
stream = System.getOut();
for (var i = 0; i < a.length; ++i)
{
    stream.write(String(a[i]), String(a[i]).length);
    stream.write("\n", 1);
}
a.length;
