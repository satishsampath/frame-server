#include <windows.h>
#include "AviFileWrapperImpl.h"

#define AVIWVERIFY(x) if (!(x)) MessageBox(NULL, L"This shouldn't happen", L"This shouldn't happen", MB_OK);

// Base class/interface for memory buffer nodes & frame nodes used below.
class AviListNode {
public:
  AviListNode(uint64_t off = 0) : offsetFromStart(off) {}
  virtual ~AviListNode() {}

  virtual int size() = 0;
  virtual int available() = 0;
  virtual void lockCapacity() = 0;
  virtual bool write(int pos, const char* data, int datalen) = 0;
  virtual bool read(int pos, char* data, int datalen) = 0;
  uint64_t getOffsetFromStart() { return offsetFromStart; }

private:
  uint64_t offsetFromStart;
};

// Holds a video or audio frame worth of data. Doesn't hold the data itself, but a reference to it.
class AviFrameListNode : public AviListNode {
public:
  AviFrameListNode(AviFrameManager* fm, bool audio, uint64_t off = 0) :
      AviListNode(off), frameManager(fm), isAudio(audio),
      frameSize(0), frameHandle(NULL) { }
  virtual ~AviFrameListNode() { frameManager->AviFrameManagerCloseHandle(frameHandle); }

  virtual int size() { return frameSize; }
  virtual int available() { AVIWVERIFY(false); return 0; }
  virtual void lockCapacity() { AVIWVERIFY(false); }
  virtual bool write(int pos, const char* data, int datalen) {
    AVIWVERIFY(pos == 0);
    frameSize = datalen;
    frameHandle = frameManager->AviFrameManagerPut(isAudio, data, datalen);
    return true;
  }
  virtual bool read(int pos, char* data, int datalen) {
    if (pos >= frameSize || pos + datalen > frameSize)
      return false;
    char* frameData;
    int frameDataLen;
    AVIWVERIFY(frameManager->AviFrameManagerGet(frameHandle, &frameData, &frameDataLen));
    memcpy(data, frameData + pos, datalen);
    //memset(data, 0, datalen);
    return true;
  }

private:
  AviFrameManager* frameManager;
  AviFrameManagerHandle frameHandle;
  bool isAudio;
  int frameSize;
};

class AviBufferListNode : public AviListNode {
public:
  AviBufferListNode(uint64_t off = 0) :
      AviListNode(off), used(0), capacity(_countof(buf)) { memset(buf, 0, capacity); }
  virtual ~AviBufferListNode() { }
  virtual int size() { return used; };
  virtual int available() { return capacity - used; }
  virtual void lockCapacity() { capacity = used; }
  virtual bool write(int pos, const char* data, int datalen) {
    if (pos >= capacity || pos + datalen > capacity)
      return false;
    memcpy(buf + pos, data, datalen);
    if (pos + datalen > used) used = pos + datalen;
    return true;
  }
  virtual bool read(int pos, char* data, int datalen) {
    if (pos >= used || pos + datalen > used)
      return false;
    memcpy(data, buf + pos, datalen);
    return true;
  }

private:
  char buf[128];
  int capacity;
  int used;
};

AviFileWrapperImpl::AviFileWrapperImpl(AviFrameManager* fm) :
    writePos(0), readPos(0), readPosInNode(0), writePosInNode(0),
    readNodeIndex(0), writeNodeIndex(0), eofbit(false), frameManager(fm) {
  nodes.push_back(new AviBufferListNode());
}

AviFileWrapperImpl::~AviFileWrapperImpl() {
  for (std::vector<AviListNode*>::iterator it = nodes.begin(); it != nodes.end(); ++it)
    delete *it;
}

void AviFileWrapperImpl::writeframe(bool isAudio, const char* data, uint64_t datalen) {
  //write(data, datalen);
    
  AviListNode* node = nodes[writeNodeIndex];
  node->lockCapacity();

  node = new AviFrameListNode(frameManager, isAudio, node->getOffsetFromStart() + node->size());
  node->write(0, data, (int) datalen);
  nodes.push_back(node);
  writeNodeIndex++;
  writePos += datalen;

  node = new AviBufferListNode(node->getOffsetFromStart() + node->size());
  nodes.push_back(node);
  writePosInNode = 0;
  writeNodeIndex++;
}

void AviFileWrapperImpl::write(const char* data, uint64_t datalen) {
  AviListNode* node = nodes[writeNodeIndex];
  while (datalen > 0) {
    int bytesToWrite = (int)datalen;
    if (writePosInNode + bytesToWrite > node->size() + node->available())
      bytesToWrite = node->size() + node->available() - writePosInNode;
    node->write(writePosInNode, data, bytesToWrite);
    data += bytesToWrite;
    datalen -= bytesToWrite;
    writePos += bytesToWrite;
    writePosInNode += bytesToWrite;
    if (datalen > 0) {
      if (writeNodeIndex == nodes.size() - 1) {
        node = new AviBufferListNode(node->getOffsetFromStart() + node->size());
        nodes.push_back(node);
      } else {
        node = nodes[writeNodeIndex + 1];
      }
      writePosInNode = 0;
      writeNodeIndex++;
    }
  }
}

void AviFileWrapperImpl::read(char* data, uint64_t datalen) {
  AviListNode* node = nodes[readNodeIndex];
  while (datalen > 0) {
    int bytesToRead = (int)datalen;
    if (readPosInNode + bytesToRead > node->size())
      bytesToRead = node->size() - readPosInNode;
    node->read(readPosInNode, data, bytesToRead);
    data += bytesToRead;
    datalen -= bytesToRead;
    readPos += bytesToRead;
    readPosInNode += bytesToRead;
    if (datalen > 0) {  // need to read more data?
      if (readNodeIndex < nodes.size() - 1) {  // got more nodes to read from?
        readNodeIndex++;
        readPosInNode = 0;
        node = nodes[readNodeIndex];
      } else {
        datalen = 0;
        eofbit = true;
      }
    }
  }
}

int AviFileWrapperImpl::nodeForPos(uint64_t pos, int left, int right) {
  // Binary search to find the node into which 'pos' falls into.
  if (right >= left) {
    int mid = left + (right - left) / 2;
    AviListNode* midNode = nodes[mid];
    if (pos < midNode->getOffsetFromStart())
      return nodeForPos(pos, left, mid - 1);
    if (pos >= midNode->getOffsetFromStart() + midNode->size())
      return nodeForPos(pos, mid + 1, right);
    return mid;
  }
  return (int)(nodes.size() - 1);
}

int AviFileWrapperImpl::nodeForPos(uint64_t pos) {
  return nodeForPos(pos, 0, (int)(nodes.size() - 1));
}

void AviFileWrapperImpl::seekp(uint64_t pos, std::ios::seekdir pt/* = std::ios::beg*/) {
  if (pt == std::ios::beg) {
    if (writePos != pos) {
      writePos = pos;
      writeNodeIndex = nodeForPos(pos);
      writePosInNode = (int)(pos - nodes[writeNodeIndex]->getOffsetFromStart());
    }
  } else {
    // only support seeking to the very end, don't care about pos.
    writeNodeIndex = (int)nodes.size() - 1;
    writePosInNode = nodes[writeNodeIndex]->size();
    writePos = nodes[writeNodeIndex]->getOffsetFromStart() + writePosInNode;
  }
}

void AviFileWrapperImpl::seekg(uint64_t pos, std::ios::seekdir pt/* = std::ios::beg*/) {
  if (pt == std::ios::beg) {
    if (readPos != pos) {
      readPos = pos;
      readNodeIndex = nodeForPos(pos);
      readPosInNode = (int)(pos - nodes[readNodeIndex]->getOffsetFromStart());
    }
  } else {
    // only support seeking to the very end, don't care about pos.
    readNodeIndex = (int)nodes.size() - 1;
    readPosInNode = nodes[readNodeIndex]->size();
    readPos = nodes[readNodeIndex]->getOffsetFromStart() + readPosInNode;
  }
}
