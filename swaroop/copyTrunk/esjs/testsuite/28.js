i = 0;
x = 0;
loop:
while (i < 10)
{
    i = i + 1;
    j = 0;
    do
    {
        j = j + 1;
        x = x + 1;
        continue loop;
    } while (j < 10);
    x = x + 1;
}
x;
