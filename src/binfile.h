#ifndef BINFILE_H
#define BINFILE_H

#include <string>

class BinFile {
public:
    BinFile(const std::string& fileName, const std::string& filePath);

    std::string getFileName() const;
    std::string getFilePath() const;

    void setFileName(const std::string& fileName);
    void setFilePath(const std::string& filePath);

private:
    std::string fileName;
    std::string filePath;
};

#endif // BINFILE_H