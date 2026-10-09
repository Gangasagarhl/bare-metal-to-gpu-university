// file_calls.cpp - names, inodes and open files on a real Linux file system.
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static struct stat info(const char* path)
{
    struct stat st {};
    if (stat(path, &st) != 0) { std::perror(path); }
    return st;
}

int main()
{
    const char text[] = "Room 12: maths\n";
    int fd = open("timetable.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644);  // a new file
    std::printf("open returned file descriptor %d\n", fd);
    if (fd < 0 || write(fd, text, sizeof text - 1) != static_cast<ssize_t>(sizeof text - 1)) {
        std::perror("write");
        return 1;
    }
    close(fd);

    const struct stat a = info("timetable.txt");
    std::printf("timetable.txt: size %lld bytes, links %llu\n", static_cast<long long>(a.st_size),
                static_cast<unsigned long long>(a.st_nlink));

    if (link("timetable.txt", "copy-for-office.txt") != 0) {  // a second name, same file
        std::perror("link");
        return 1;
    }
    const struct stat b = info("copy-for-office.txt");
    std::printf("after link: links %llu, same inode as timetable.txt: %s\n",
                static_cast<unsigned long long>(b.st_nlink), a.st_ino == b.st_ino ? "yes" : "no");

    if (rename("timetable.txt", "week1.txt") != 0) {  // a new name, same file
        std::perror("rename");
        return 1;
    }
    const struct stat c = info("week1.txt");
    std::printf("after rename: same inode: %s\n", c.st_ino == a.st_ino ? "yes" : "no");

    fd = open("week1.txt", O_RDONLY);                                      // keep the file open...
    if (unlink("week1.txt") != 0 || unlink("copy-for-office.txt") != 0) {  // ...remove both names
        std::perror("unlink");
        return 1;
    }
    struct stat d {};
    fstat(fd, &d);
    std::printf("both names removed, file still open: links %llu\n",
                static_cast<unsigned long long>(d.st_nlink));
    char buf[64] = {};
    const ssize_t n = read(fd, buf, sizeof buf - 1);
    std::printf("read %zd bytes through the open descriptor: %s", n, buf);
    close(fd);  // now the kernel may free it
    std::printf("after close, the name exists: %s\n",
                access("week1.txt", F_OK) == 0 ? "yes" : "no");
    return 0;
}
