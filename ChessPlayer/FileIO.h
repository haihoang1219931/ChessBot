#ifndef FILEIO_H
#define FILEIO_H

// fileio.h
#include <QObject>
#include <QFile>
#include <QTextStream>

class FileIO : public QObject {
    Q_OBJECT
public:
    explicit FileIO(QObject *parent = nullptr);

    Q_INVOKABLE bool write(const QString& filename, const QString& data);
    Q_INVOKABLE QString read(const QString& filename);
};


#endif // FILEIO_H
