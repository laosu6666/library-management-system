#include <QApplication>
#include "library.h"
#include "mainwindow.h"
#include "database.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 初始化数据库
    Database* db = Database::instance();
    if(!db->initialize()) {
        qDebug() << "Failed to initialize database!";
        return 1;
    }

    // 创建图书馆系统
    Library library;

    // 显示主窗口
    MainWindow w(&library);
    w.show();

    return a.exec();
}
