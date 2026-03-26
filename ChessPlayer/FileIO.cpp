#include "FileIO.h"

FileIO::FileIO(QObject *parent) : QObject(parent)
{

}

Q_INVOKABLE bool FileIO::write(const QString& filename, const QString& data) {
    QFile file(filename);
    if (file.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream out(&file);
        out << data;
        file.close();
        return true;
    }
    return false;
}

Q_INVOKABLE QString FileIO::read(const QString& filename) {
    QFile file(filename);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&file);
        return in.readAll();
    }
    return "";
}
