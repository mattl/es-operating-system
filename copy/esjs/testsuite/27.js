i = 0;
x = 0;
loop:
while (i < 10)
{
    i = i + 1;
    j = 0;
    while (j < 10)
    {
        j = j + 1;
        continue loop;
        x = x + 1;
    }
    x = x + 1;
}
x;
