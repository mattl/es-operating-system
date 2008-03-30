stdin = System.input;
stdout = System.output;
root = System.root;

function readLine(prompt)
{
    stdout.write(prompt, prompt.length);
    var line = "";
    var c;
    while ((c = stdin.read(1)) != '')
    {
        if (c == '\n')
        {
            return line;
        }
        line += c;
    }
    throw "EOF";
}

for (;;)
{
    var line = readLine("% ");
    try
    {
        var file = File(root.lookup(line));
        var stream = file.getStream();
        var x = "";
        while ((c = stream.read(255)) != '')
        {
            x += c;
        }
        x = eval(x);
    }
    catch (e)
    {
        x = e;
    }
    result = String(x);
    stdout.write(result + '\n', result.length + 1);
}
"Ok";
