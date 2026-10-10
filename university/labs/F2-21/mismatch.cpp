// Does not compile with the course flags: memory from new[] released with plain delete.
int main()
{
    int* seats = new int[8]{};
    seats[0] = 1;
    const int first = seats[0];
    delete seats;
    return first - 1;
}
