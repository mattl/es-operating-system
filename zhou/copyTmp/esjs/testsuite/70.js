var stdout = System.output;
var s = "こんにちは, 世界.";
var pos = 0;
var c;
while (c = s.charAt(pos))
{
    stdout.write("'" + c + "'\n", c.length + 3);
    pos += c.length;
}
pos;
