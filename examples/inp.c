void _printf(char *, ...);
int main()
{
    int i = 10;

    for (int i = 0; i < 5; i = i + 1)
    {
        int i = 2;
        _printf("%u ", i);
    }

    _printf("%u ", i);
    return 0;
}
