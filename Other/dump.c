
int main()
{
    char* p = malloc(10);
    free(p);
    free(p);
    
    return 0;
}

