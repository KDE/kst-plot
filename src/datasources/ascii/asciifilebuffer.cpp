/***************************************************************************
 *                                                                         *
 *   Copyright : (C) 2012 Peter Kümmel                                     *
 *   email     : syntheticpp@gmx.net                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "asciifilebuffer.h"
#include "debug.h"

#include <QFile>
#include <QDebug>
#include <QVarLengthArray>


//-------------------------------------------------------------------------------------------
extern int MB;
extern size_t maxAllocate;

//-------------------------------------------------------------------------------------------
AsciiFileBuffer::AsciiFileBuffer() : 
  _file(0), _begin(-1), _bytesRead(0)
{
}

//-------------------------------------------------------------------------------------------
AsciiFileBuffer::~AsciiFileBuffer()
{
  clear();
}

//-------------------------------------------------------------------------------------------
void AsciiFileBuffer::setFile(QFile* file)
{
  delete _file;
  _file = file; 
}

//-------------------------------------------------------------------------------------------
bool AsciiFileBuffer::openFile(QFile &file) 
{
  // Don't use 'QIODevice::Text'!
  // Because CR LF line ending breaks row offset calculation
  return file.open(QIODevice::ReadOnly);
}

//-------------------------------------------------------------------------------------------
bool AsciiFileBuffer::reOpenFile(QFile &file)
{
  // Don't use 'QIODevice::Text'!
  // Because CR LF line ending breaks row offset calculation
  file.close();
  return file.open(QIODevice::ReadOnly);
}

//-------------------------------------------------------------------------------------------
void AsciiFileBuffer::clear()
{
  _fileData.clear();
  _begin = -1;
  _bytesRead = 0;
}

//-------------------------------------------------------------------------------------------
qint64 AsciiFileBuffer::findRowOfPosition(const AsciiFileBuffer::RowIndex& rowIndex, qint64 searchStart, qint64 pos) const
{
  if (rowIndex.isEmpty() ||
      pos < 0 || pos >= rowIndex[rowIndex.size() - 1] || // within the file
      searchStart > rowIndex.size()-1 || pos < rowIndex[searchStart]) //within the search region
    return -1;

  // get close with a binary search...
  const qint64 indexOfLastRow = rowIndex.size() - 2;

  qint64 i0 = searchStart;
  qint64 i1 = indexOfLastRow;
  qint64 im = (i0+i1)/2;

  while (i1 -  i0 > 1L ) {
    if (pos < rowIndex[im]) {
      i1 = im;
    } else {
      i0 = im;
    }
    im = (i0+i1)/2;
  }

  // now find the exact row... (FIXME - could be cleaner!)
  im = qMax(im-4, searchStart);
  for (qint64 row = im; row <= indexOfLastRow; ++row) {
    if (pos < rowIndex[row]) {
      return row - 1;
    }
  }
  if (pos < rowIndex[indexOfLastRow + 1]) // length of file in the last element
    return indexOfLastRow;
  return -1;
}

//-------------------------------------------------------------------------------------------
const QVector<AsciiFileData> AsciiFileBuffer::splitFile(qint64 chunkSize, const RowIndex& rowIndex, qint64 start, qint64 bytesToRead) const
{
  const qint64 end = start + bytesToRead; // position behind last valid seekable byte in file
  if (chunkSize <= 0 || rowIndex.isEmpty() || start >= end || start < 0
      || bytesToRead <= 0 || start + bytesToRead > rowIndex[rowIndex.size() - 1])
    return QVector<AsciiFileData>();

  qint64 nextRow = findRowOfPosition(rowIndex, 0, start);
  QVector<AsciiFileData> chunks;
  chunks.reserve(bytesToRead / chunkSize);
  qint64 pos = start;
  qint64 rows = rowIndex.size();
  while (pos < end) {
    // use for storing reading information only
    AsciiFileData chunk;
    // error if chunkSize is too small for one row
    if (nextRow + 1 < rows && rowIndex[nextRow + 1] - rowIndex[nextRow] > chunkSize)
      return  QVector<AsciiFileData>();
    // read complete chunk or to end of file
    qint64 endRead = (pos + chunkSize < end ? pos + chunkSize : end);
    // adjust to row end: pos + chunkRead is in the middle of a row, find index of this row
    const qint64 rowBegin = nextRow;
    nextRow = findRowOfPosition(rowIndex, nextRow, endRead - 1);
    if (nextRow == -1 || nextRow >= rows)
      return  QVector<AsciiFileData>();
    // read until the beginning of the found row
    if (nextRow == rows - 2) { // last valid row
      // if exactly at the end of the row, read this row
      if (endRead == rowIndex[rows - 1]) {
        nextRow++;
        endRead = end;
      }  else {
        // find complete last row next time
        endRead = end - 1;
      }
    } else {
      // if exactly at the end of the row, read this row
      if (endRead == rowIndex[nextRow + 1])
        nextRow++;
      endRead = rowIndex[nextRow];
    }
    // set information about positions in the file
    chunk.setBegin(rowIndex[rowBegin]);
    chunk.setBytesRead(rowIndex[nextRow] - rowIndex[rowBegin]);
    // set information about rows
    chunk.setRowBegin(rowBegin);
    chunk.setRowsRead(nextRow - rowBegin);
    chunks << chunk;
    pos = rowIndex[nextRow];
  }
  //qDebug() << "File split into " << chunks.size() << " chunks:"; AsciiFileData::logData(chunks);
  return chunks;
}

//-------------------------------------------------------------------------------------------
void AsciiFileBuffer::useOneWindowWithChunks(const RowIndex& rowIndex, qint64 start, qint64 bytesToRead, int numChunks)
{
  useSlidingWindowWithChunks(rowIndex, start, bytesToRead, bytesToRead, numChunks, false);
}

//-------------------------------------------------------------------------------------------
void AsciiFileBuffer::useSlidingWindow(const RowIndex& rowIndex, qint64 start, qint64 bytesToRead, qint64 windowSize)
{
  useSlidingWindowWithChunks(rowIndex, start, bytesToRead, windowSize, 1, true);
}

//-------------------------------------------------------------------------------------------
void AsciiFileBuffer::useSlidingWindowWithChunks(const RowIndex& rowIndex, qint64 start, qint64 bytesToRead, qint64 windowSize, int numWindowChunks)
{
  useSlidingWindowWithChunks(rowIndex, start, bytesToRead, windowSize, numWindowChunks, true);
}

//-------------------------------------------------------------------------------------------
void AsciiFileBuffer::useSlidingWindowWithChunks(const RowIndex& rowIndex, qint64 start, qint64 bytesToRead, qint64 windowSize, int numWindowChunks, bool reread)
{
  clear();
  if (!_file)
    return;

  if (bytesToRead <= 0 || numWindowChunks <= 0 || windowSize <= 0)
    return;

  qint64 chunkSize = windowSize / numWindowChunks;
  QVector<AsciiFileData> chunks = splitFile(chunkSize, rowIndex, start, bytesToRead);
  // chunks.size() could be greater than numWindowChunks!

  // no sliding window
  if (bytesToRead == windowSize)
  {
    for (int i = 0; i < chunks.size(); i++) {
      chunks[i].setFile(_file);
      chunks[i].setReread(reread);
      _bytesRead += chunks[i].bytesRead();
    }
    _fileData.push_back(chunks);
  }
  else
  {
    // sliding window
    // prepare window with numSubChunks chunks
    QVector<AsciiFileData> window;
    window.reserve(numWindowChunks);
    for (int i = 0; i < numWindowChunks; i++) {
      AsciiFileData sharedArray;
      if (!sharedArray.resize(chunkSize)) {
        Kst::Debug::self()->log(QString("AsciiFileBuffer: not enough memory available for sliding window"));
        return;
      }
      sharedArray.setFile(_file);
      window.push_back(sharedArray);
    }

    _fileData.reserve(bytesToRead / windowSize);
    int i = 0;
    while (i < chunks.size()) {
      QVector<AsciiFileData> windowChunks;
      windowChunks.reserve(window.size());
      for (int s = 0; s < window.size(); s++) {
        AsciiFileData chunk = chunks[i];
        chunk.setSharedArray(window[s]);
        chunk.setFile(_file);
        chunk.setReread(reread);
        _bytesRead += chunk.bytesRead();
        windowChunks.push_back(chunk);
        i++;
        if (i >= chunks.size())
          break;
      }
      // each entry is one slide of the window
      _fileData.push_back(windowChunks);
      //qDebug() << "Window chunks:"; AsciiFileData::logData(windowChunks);
    }
  }

  _begin = start;
  if (_bytesRead != bytesToRead) {
    clear();
    Kst::Debug::self()->log(QString("AsciiFileBuffer: error while splitting into file %1 chunks").arg(_fileData.size()));
  }
}

//-------------------------------------------------------------------------------------------
bool AsciiFileBuffer::readWindow(QVector<AsciiFileData>& window) const
{
  for (int i = 0; i < window.size(); i++) {
    if (!window[i].read()) {
      return false;
    }
  }
  return true;
}

