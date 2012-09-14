int main(int argc, char *argv[])
{
    assert(argc == 1 + 1);
    const unsigned long count = atol(argv[1]);
    for (unsigned long i = 0; i < count; ++i)
    {
        fillAll();
        writeAll();
        clearAll();
        readAll();
        validateAll();
    }
    return 0;
}
