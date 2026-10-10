// F11-17 forensic evidence generator: "The job that read someone else's data".
// A model of a shared scratch file system with POSIX-style permission bits (owner, group,
// others; read/write/execute) and the scheduler's job records. Two research groups share
// the cluster. SYNTHETIC users, jobs and times; the permission decisions are computed by
// the same rule as the kernel's basic check (owner bits if you own the file, else group
// bits if you are in its group, else the "others" bits), without ACLs or capabilities.
// The directories are printed as listed; the model does not check search (x) permission.
#include <cstdio>
#include <string>
#include <vector>

struct File
{
    std::string path, owner, group;
    unsigned mode;   // e.g. 0644
};

struct User
{
    std::string name, group;
};

bool mayRead(const User& u, const File& f)
{
    if (u.name == f.owner) {
        return (f.mode & 0400u) != 0;
    }
    if (u.group == f.group) {
        return (f.mode & 0040u) != 0;
    }
    return (f.mode & 0004u) != 0;
}

std::string modeText(unsigned m)
{
    std::string s = "-";
    const char* rwx = "rwx";
    for (int i = 8; i >= 0; --i) {
        s += (m >> i) & 1u ? rwx[(8 - i) % 3] : '-';
    }
    return s;
}

int main()
{
    const User amara{"amara", "vision-lab"};
    const User bo{"bo", "robot-lab"};
    const unsigned umask = 0022;    // the job environment's file-creation mask
    std::vector<File> files;
    // amara's training job writes its results; the program asks for mode 0666 & ~umask
    files.push_back({"/scratch/amara/run7/model.bin", "amara", "vision-lab", 0666u & ~umask});
    files.push_back({"/scratch/amara/run7/dataset-index.csv", "amara", "vision-lab",
                     0666u & ~umask});
    files.push_back({"/scratch/amara/keys/storage-token.txt", "amara", "vision-lab", 0600u});
    std::printf("== scheduler job records ==\n");
    std::printf("job 4411  user amara  group vision-lab  start 09:12  end 10:47  node compute2\n");
    std::printf("job 4419  user bo     group robot-lab   start 10:55  end 10:58  node compute1\n");
    std::printf("\n== scratch listing (mode owner group path), umask of job 4411 = %04o ==\n",
                umask);
    std::printf("drwxr-xr-x amara vision-lab /scratch/amara\n");
    std::printf("drwxr-xr-x amara vision-lab /scratch/amara/run7\n");
    std::printf("drwxr-xr-x amara vision-lab /scratch/amara/keys\n");
    for (const File& f : files) {
        std::printf("%s amara vision-lab %s\n", modeText(f.mode).c_str(), f.path.c_str());
    }
    std::printf("\n== file-access audit, job 4419 (user bo) ==\n");
    const char* times[] = {"10:56:02", "10:56:03", "10:56:05"};
    int i = 0;
    for (const File& f : files) {
        std::printf("%s  open-read  %-40s %s\n", times[i++], f.path.c_str(),
                    mayRead(bo, f) ? "allowed" : "denied");
    }
    return 0;
}
