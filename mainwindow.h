#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSql>
#include <QSqlTableModel>
#include <QTableView>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    QSqlDatabase db;

    //  МОДЕЛИ 
    QSqlTableModel *model = nullptr;        // собаки
    QSqlTableModel *diseaseModel = nullptr; // болезни
    QSqlTableModel *exhibModel = nullptr;   // выставки
    QSqlTableModel *parentModel = nullptr;  // родители
    QSqlTableModel *breedModel = nullptr;   // породы
    QSqlTableModel *historyModel = nullptr; // история болезней
    QSqlTableModel *matingModel = nullptr;  // вязки


    //  ФУНКЦИИ 
    void connectToDB();

    void setupTable();
    void setupDiseaseTable();
    void setupExhibitions();
    void setupParents();
    void setupBreeds();
    void setupHistory();
    void setupMating();

    void loadBreeds();


    QSqlTableModel* getCurrentModel();
    QTableView* getCurrentView();
};

#endif // MAINWINDOW_H




