#ifndef AVI_FILE_WRAPPER_IMPL_H
#define AVI_FILE_WRAPPER_IMPL_H

#include <AVI20/AviFileWrapper.h>

#include <windows.h>
#include <fstream>
#include <vector>

class AviListNode;

typedef void* AviFrameManagerHandle;

class AviFrameManager {
public:
  virtual AviFrameManagerHandle AviFrameManagerPut(bool isAudio, const char* data, int datalen) = 0;
  virtual bool AviFrameManagerGet(AviFrameManagerHandle handle, char** data, int* datalen) = 0;
  virtual void AviFrameManagerCloseHandle(AviFrameManagerHandle handle) = 0;
};

class AviFileWrapperImpl : public AviFileWrapper {
public:
  AviFileWrapperImpl(AviFrameManager* fm);
  virtual ~AviFileWrapperImpl();

  void writeframe(bool isAudio, const char* data, uint64_t datalen);
  void write(const char* data, uint64_t datalen);
  void read(char* data, uint64_t datalen);
  void seekp(uint64_t pos, std::ios::seekdir pt = std::ios::beg);
  void seekg(uint64_t pos, std::ios::seekdir pt = std::ios::beg);
  uint64_t tellp() { return writePos; }
  uint64_t tellg() { return readPos; }
  bool good() { return !eofbit; }
  void clear() { eofbit = false; }

private:
  int nodeForPos(uint64_t pos, int left, int right);
  int nodeForPos(uint64_t pos);

  AviFrameManager* frameManager;
  std::vector<AviListNode*> nodes;
  uint64_t writePos;
  int writeNodeIndex;
  int writePosInNode;
  uint64_t readPos;
  int readNodeIndex;
  int readPosInNode;
  bool eofbit;
};

class AviFileDiskWrapperImpl : public AviFileWrapper {
public:
  AviFileDiskWrapperImpl(const wchar_t* filepath, bool truncate) {
    unsigned int mode = std::ios::in | std::ios::out | std::ios::binary;
    if (truncate)
      mode = mode | std::ios::trunc;
    aviFile.open(filepath, mode);
  }
  virtual ~AviFileDiskWrapperImpl() { close(); }
  bool is_open() { return aviFile.is_open(); }
  void close() { aviFile.close(); }

  void writeframe(bool isAudio, const char* data, uint64_t datalen) { return write(data, datalen); }
  void write(const char* data, uint64_t datalen) { aviFile.write(data, datalen); }
  void read(char* data, uint64_t datalen) { aviFile.read(data, datalen); }
  void seekp(uint64_t pos, std::ios::seekdir pt = std::ios::beg) { aviFile.seekp(pos, pt); }
  void seekg(uint64_t pos, std::ios::seekdir pt = std::ios::beg) { aviFile.seekg(pos, pt); }
  uint64_t tellp() { return aviFile.tellp(); }
  uint64_t tellg() { return aviFile.tellg(); }
  bool good() { return aviFile.good(); }
  void clear() { aviFile.clear();  }

private:
  std::fstream aviFile;
};

#endif  // AVI_FILE_WRAPPER_IMPL_H