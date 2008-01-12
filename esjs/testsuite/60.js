stdin = System.getIn();
stdout = System.getOut();

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
    line = readLine("% ");
    stdout.write(line + '\n', line.length + 1);
}
