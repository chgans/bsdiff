#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QDebug>

#include <iostream>

int default_max_diff_report = 50;

int default_diff_ratio = 60;
int default_max_diff_len = 32;
int default_diff_ctx_len = 32;

int default_max_bit_shifts = 80;

int default_match_ratio = 90;
int default_min_match_length = 20;

void dumpFile(const QString &filename)
{
    QFile file(filename);
    file.open(QFile::ReadOnly);
    QByteArray data = file.readAll();
    file.close();
    for (int i=0; i<data.size(); i++) {
        for (unsigned char j=128; j>0; j /= 2) {
            std::cout << ((data.at(i) & j) ? "1" : "0");
        }
    }
    std::cout << std::endl;
}

void testBitAt();
bool bitAt(const QByteArray &bytes, int pos)
{
    return (bytes.at(pos/8) & (1<<(7-(pos%8)))) != 0;
}

void testFindNextDiff();
int findNextDiff(const QByteArray &bytes1, int pos1,
                    const QByteArray &bytes2, int pos2)
{
    int n = 0;
    while ( (pos1+n) < (8*bytes1.count()) &&
            (pos2+n) < (8*bytes2.count()) &&
            (bitAt(bytes1, pos1+n) == bitAt(bytes2, pos2+n)) )
        n++;
    return n;
}

void testDiffLength();
int diffLength(const QByteArray &bytes1, int pos1,
               const QByteArray &bytes2, int pos2,
               int maxlen, int ratio)
{
  int n = 0;
  int ndiff = 0;
  while ( (pos1+n) < (8*bytes1.count()) &&
          (pos2+n) < (8*bytes2.count()) &&
          (n < maxlen) ) {
      if (bitAt(bytes1, pos1+n) != bitAt(bytes2, pos2+n)) {
          ndiff+=1;
      }
      if ((100*(ndiff+1))/(n+1) > ratio)
          n++;
      else
          break;
  }
  if ((pos1+n) >= (8*bytes1.count())) {
      return qMin(8*bytes2.count() - pos2, maxlen);
  }
  if ((pos2+n) >= (8*bytes2.count())) {
      return qMin(8*bytes1.count() - pos1, maxlen);
  }
  return n;
}


int matchLength(const QByteArray &bytes1, int pos1,
                const QByteArray &bytes2, int pos2,
                int minlen, int ratio)
{
  int n = 0;
  int nmatch = 0;
  while ( (pos1+n) < (8*bytes1.count()) &&
          (pos2+n) < (8*bytes2.count()) &&
          (n < minlen) ) {
      if (bitAt(bytes1, pos1+n) == bitAt(bytes2, pos2+n)) {
          nmatch+=1;
      }
      if ((100*(nmatch+1))/(n+1) > ratio)
          n++;
      else
          break;
  }
  return n;
}

void dumpDiff(const QByteArray &data1, int pos1,
              const QByteArray &data2, int pos2,
              int len, int /*ctxlen*/)
{

    std::cout << QString("@@ -%1,%2 +%3,%4 @@")
                 .arg(pos1, 8, 16, QLatin1Char('0'))
                 .arg(len, 8, 16, QLatin1Char('0'))
                 .arg(pos2, 8, 16, QLatin1Char('0'))
                 .arg(len, 8, 16, QLatin1Char('0'))
                 .toStdString() << std::endl;
    int l1 = qMin(len, 8*data1.size() - pos1);
    for (int j = 0; j < l1; j++)
        std::cout << (bitAt(data1, pos1 + j) == true ? "1" : "0");
    std::cout << std::endl;
    int l2 = qMin(len, 8*data2.size() - pos2);
    for (int j = 0; j < l2; j++)
        std::cout << (bitAt(data2, pos2 + j) == true ? "1" : "0");
    std::cout << std::endl;
    int l = qMin(l1, l2);
    for (int j = 0; j < l; j++)
        std::cout << (bitAt(data1, pos1 + j) == bitAt(data2, pos2 + j) ? " " : "^");
    std::cout << std::endl;
}

void dumpShift(const QByteArray &data1, int pos1,
               const QByteArray &data2, int pos2,
               int len, int shift)
{/*
    if (shift == 0)
        return;*/

    int shift1 = 0;
    int shift2 = 0;
    if (shift > 0)
        shift1 = shift;
    else
        shift2 = shift;


    std::cout << QString("@@ [%5,%6] -%1,%2 +%3,%4 @@")
                 .arg(pos1+1/*, 8, 16, QLatin1Char('0')*/)
                 .arg(shift2 - len)
                 .arg(pos2+1/*, 8, 16, QLatin1Char('0')*/)
                 .arg(len + shift1)
                 .arg(len)
                 .arg(shift)
                 .toStdString() << std::endl;

    int l1 = qMin(len - shift2, 8*data1.size() - pos1);
    if (l1 > 0) {
        std::cout << "-";
        for (int s=0; s<shift1; s++)
            std::cout << " ";
        for (int j = 0; j < l1; j++)
            std::cout << (bitAt(data1, pos1 + j) == true ? "1" : "0");
        std::cout << std::endl;
    }

    int l2 = qMin(len + shift1, 8*data2.size() - pos2);
    if (l2 > 0) {
        std::cout << "+";
        for (int s=0; s>shift2; s--)
            std::cout << " ";
        for (int j = 0; j < l2; j++)
            std::cout << (bitAt(data2, pos2 + j) == true ? "1" : "0");
        std::cout << std::endl;
    }

    std::cout << " ";
    for (int j = 0; j < qAbs(shift); j++)
        std::cout << "^";
    l1 = qMin(len + qAbs(shift) - shift2, 8*data1.size() - pos1);
    l2 = qMin(len + qAbs(shift) + shift1, 8*data2.size() - pos2);
    int l = qMin(l1, l2);
    for (int j = 0; j < l; j++)
        std::cout << (bitAt(data1, pos1 - shift2 + j) == bitAt(data2, pos2 + shift1 + j) ? " " : "^");
    std::cout << std::endl;
}


int findShift(const QByteArray &data1, int pos1,
              const QByteArray &data2, int pos2,
              int max_shift,
              int min_match,
              int match_ratio)
{
    // right-shift from
    for (int i=0; i <= max_shift; i++) {
        if (matchLength(data1, pos1, data2, pos2 + i, min_match, match_ratio) == min_match) {
            return i;
        }
    }
    // left-shift from
    for (int i=0; i <= max_shift; i++) {
        if (matchLength(data1, pos1 + i, data2, pos2, min_match, match_ratio) == min_match) {
            return -i;
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    testBitAt();
    testFindNextDiff();
    testDiffLength();

    QString filename1 = QString::fromLocal8Bit(argv[1]);
    QFile file1(filename1);
    file1.open(QFile::ReadOnly);
    QByteArray data1 = file1.readAll();
    file1.close();
    QString filename2 = QString::fromLocal8Bit(argv[2]);
    QFile file2(filename2);
    file2.open(QFile::ReadOnly);
    QByteArray data2 = file2.readAll();
    file2.close();

    std::cout << "--- " << filename1.toStdString() << std::endl;
    std::cout << "+++ " << filename2.toStdString() << std::endl;

    int len1 = 8*data1.size();
    int pos1 = 0;
    int len2 = 8*data2.size();
    int pos2 = 0;
    int i, l;
    int n = 0;
    while (pos1 < len1 && pos2 < len2) {
        i = findNextDiff(data1, pos1, data2, pos2);
        pos1 += i;
        pos2 += i;

        l = diffLength(data1, pos1, data2, pos2,
                       default_max_diff_len, default_match_ratio);
//        dumpDiff(data1, pos1, data2, pos2, l,
//                 default_diff_ctx_len);

        int s = findShift(data1, pos1, data2, pos2,
                          default_max_bit_shifts,
                          default_min_match_length,
                          default_match_ratio);
        dumpShift(data1, pos1, data2, pos2, l, s);

        if (s > 0) {
            pos2 += s;
        }
        else if (s < 0) {
            pos1 -= s;
        }

        if (n++ > default_max_diff_report)
            break;

        pos1 += l;
        pos2 += l;
    }

    return 0;
}


void testBitAt()
{
    //qDebug() << "Testing bitAt()...";
    QByteArray bytes;
    int pos = 0;

    bytes.append(0x8A).append(0x56);

    Q_ASSERT(bitAt(bytes, pos++) == true);
    Q_ASSERT(bitAt(bytes, pos++) == false);
    Q_ASSERT(bitAt(bytes, pos++) == false);
    Q_ASSERT(bitAt(bytes, pos++) == false);

    Q_ASSERT(bitAt(bytes, pos++) == true);
    Q_ASSERT(bitAt(bytes, pos++) == false);
    Q_ASSERT(bitAt(bytes, pos++) == true);
    Q_ASSERT(bitAt(bytes, pos++) == false);

    Q_ASSERT(bitAt(bytes, pos++) == false);
    Q_ASSERT(bitAt(bytes, pos++) == true);
    Q_ASSERT(bitAt(bytes, pos++) == false);
    Q_ASSERT(bitAt(bytes, pos++) == true);

    Q_ASSERT(bitAt(bytes, pos++) == false);
    Q_ASSERT(bitAt(bytes, pos++) == true);
    Q_ASSERT(bitAt(bytes, pos++) == true);
    Q_ASSERT(bitAt(bytes, pos++) == false);
}


void testFindNextDiff()
{
    //qDebug() << "Testing findNextDiff()...";
    QByteArray b1;
    b1.append(0x55).append(0xaa).append(0x35);
    QByteArray b2;
    b2.append(0x54).append(0x2a).append(0x53);

    Q_ASSERT(findNextDiff(b1, 0, b2, 0) == 7);
    Q_ASSERT(findNextDiff(b1, 8, b2, 8) == 0);
    Q_ASSERT(findNextDiff(b1, 0, b2, 16) == 5);
    Q_ASSERT(findNextDiff(b1, 24, b2, 0) == 0);
    Q_ASSERT(findNextDiff(b1, 27, b2, 54) == 0);
    Q_ASSERT(findNextDiff(b1, 20, b2, 16) == 4);
}

void testDiffLength()
{
    //qDebug() << "Testing diffLength()...";

    QByteArray b1;
    b1.append(0xff).append(0xff).append(0xff);
    QByteArray b2;
    b2.append(~b1.at(0)).append(~b1.at(1)).append(~b1.at(2));

    Q_ASSERT(diffLength(b1, 0, b2, 0,  100, 100) == 24);
    Q_ASSERT(diffLength(b1, 0, b2, 0,  11,  100) == 11);
    Q_ASSERT(diffLength(b1, 0, b2, 0,  24,  100) == 24);
    Q_ASSERT(diffLength(b1, 0, b2, 16, 100, 100) == 24);
    Q_ASSERT(diffLength(b1, 0, b2, 16, 5,   100) == 5);
    Q_ASSERT(diffLength(b1, 8, b2, 16, 100, 100) == 16);
    Q_ASSERT(diffLength(b1, 8, b2, 16, 15,  100) == 15);

    QByteArray b0;
    b0.append(0x55).append(0xaa).append(0x35);
    Q_ASSERT(diffLength(b0, 0, b0, 0, 100, 100) == 0);
    Q_ASSERT(diffLength(b0, 0, b0, 0, 3,   100) == 0);
    Q_ASSERT(diffLength(b0, 0, b0, 8, 100, 100) == 9);
    Q_ASSERT(diffLength(b0, 0, b0, 8, 3,   100) == 3);

}
