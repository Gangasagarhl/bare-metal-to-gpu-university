// wild_pointer.cpp - the forensic lab's crashing grade book.
#include <cstdio>
#include <string>
#include <vector>

struct Student {
    std::string name;  // bytes 0..31 of a Student on this platform
    int marks = 0;
};

Student* find(std::vector<Student>& cls, const std::string& name)
{
    for (Student& s : cls) {
        if (s.name == name) { return &s; }
    }
    return nullptr;  // "not found"
}

int main()
{
    std::vector<Student> cls{{"Amara", 71}, {"Bo", 64}};
    std::printf("Amara: %d\n", find(cls, "Amara")->marks);
    std::printf("Chen: %d\n", find(cls, "Chen")->marks);  // Chen is not in this class
    return 0;
}
