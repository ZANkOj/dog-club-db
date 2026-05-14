#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDate>
#include <QItemSelectionModel>

// КОНСТРУКТОР

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


        //  ПОДКЛЮЧЕНИЕ К БД

        connectToDB();

    //  ТАБЛИЦЫ

    setupTable();
    setupBreeds();
    setupExhibitions();
    setupParents();
    setupMating();
    setupDiseaseTable();
    setupHistory();


    // 1:M СОБАКА -> ИСТОРИЯ БОЛЕЗНЕЙ

    loadBreeds();

    // СВЯЗЬ 1:M

    connect(ui->tableDogs,
            &QTableView::doubleClicked,
            this,
            [this](const QModelIndex &index)
            {
                if(!index.isValid())
                    return;

                int row = index.row();

                int dogId =
                    model->data(
                             model->index(row,0)
                             ).toInt();

                QString dogName =
                    model->data(
                             model->index(row,2)
                             ).toString();

                QDialog *dialog =
                    new QDialog(this);

                dialog->setWindowTitle(
                    "История болезней: "
                    + dogName);

                dialog->resize(700,400);

                QTableView *view =
                    new QTableView(dialog);

                QSqlTableModel *tempModel =
                    new QSqlTableModel(dialog, db);

                tempModel->setTable("\"история болезней\"");

                tempModel->setEditStrategy(
                    QSqlTableModel::OnFieldChange);

                tempModel->setFilter(
                    QString("\"код собаки\" = %1")
                        .arg(dogId));

                tempModel->select();

                view->setModel(tempModel);

                view->setSortingEnabled(true);

                view->resizeColumnsToContents();

                QPushButton *btnAdd =
                    new QPushButton("Добавить");

                QPushButton *btnDelete =
                    new QPushButton("Удалить");

                QPushButton *btnClose =
                    new QPushButton("Закрыть");

                connect(btnAdd,
                        &QPushButton::clicked,
                        dialog,
                        [tempModel, dogId, view]()
                        {
                            int row =
                                tempModel->rowCount();

                            tempModel->insertRow(row);

                            tempModel->setData(
                                tempModel->index(row,1),
                                dogId);

                            tempModel->setData(
                                tempModel->index(row,2),
                                1);

                            tempModel->setData(
                                tempModel->index(row,3),
                                QDate::currentDate());

                            view->selectRow(row);
                        });

                connect(btnDelete,
                        &QPushButton::clicked,
                        dialog,
                        [tempModel, view]()
                        {
                            QModelIndexList rows =
                                view->selectionModel()
                                    ->selectedRows();

                            for(const QModelIndex &index : rows)
                                tempModel->removeRow(index.row());

                            tempModel->submitAll();

                            tempModel->select();
                        });

                connect(btnClose,
                        &QPushButton::clicked,
                        dialog,
                        &QDialog::close);

                QHBoxLayout *buttons =
                    new QHBoxLayout();

                buttons->addWidget(btnAdd);
                buttons->addWidget(btnDelete);
                buttons->addWidget(btnClose);

                QVBoxLayout *layout =
                    new QVBoxLayout(dialog);

                layout->addWidget(view);

                layout->addLayout(buttons);

                dialog->setLayout(layout);

                dialog->exec();
            });


    // СОБАКИ

    connect(ui->btnAdd,
            &QPushButton::clicked,
            this,
            [this]()
            {
                QSqlQuery q;

                q.exec("SELECT COALESCE(MAX(\"код собаки\"),0)+1 FROM \"собаки\"");

                int nextId = 1;

                if(q.next())
                    nextId = q.value(0).toInt();

                int row = model->rowCount();

                model->insertRow(row);

                model->setData(model->index(row,0), nextId);
                model->setData(model->index(row,1), 1);
                model->setData(model->index(row,2), "Новая собака");
                model->setData(model->index(row,3), "M");
                model->setData(model->index(row,4), "Владелец...");
                model->setData(model->index(row,5), "Адрес...");
                model->setData(model->index(row,6), true);
                model->setData(model->index(row,7), 5);
                model->setData(model->index(row,8), QDate::currentDate());

                model->submitAll();
                model->select();
            });

    connect(ui->btnDelete,
            &QPushButton::clicked,
            this,
            [this]()
            {
                QModelIndexList rows =
                    ui->tableDogs->selectionModel()->selectedRows();

                for(const QModelIndex &index : rows)
                    model->removeRow(index.row());

                model->submitAll();
                model->select();
            });

    connect(ui->btnRefresh,
            &QPushButton::clicked,
            this,
            [this]()
            {
                model->select();
            });

    // ФИЛЬТР СОБАК

    ui->comboFilterDogsCondition->addItems({
        "=",
        ">",
        "<",
        "содержит"
    });

    ui->comboFilterDogsField->addItems({
        "кличка",
        "владелец",
        "адрес",
        "пол",
        "оценка психики"
    });

    connect(ui->btnFilterDogs,
            &QPushButton::clicked,
            this,
            [this]()
            {
                QString field = ui->comboFilterDogsField->currentText();
                QString condition = ui->comboFilterDogsCondition->currentText();
                QString value = ui->lineFilterDogsValue->text();

                QString filter;

                if(condition == "содержит")
                {
                    filter = QString("\"%1\" ILIKE '%%2%'")
                    .arg(field)
                        .arg(value);
                }
                else
                {
                    filter = QString("\"%1\" %2 '%3'")
                    .arg(field)
                        .arg(condition)
                        .arg(value);
                }

                model->setFilter(filter);
                model->select();
            });

    // ФИЛЬТР ПОРОД

    ui->comboFilterBreedCondition->addItems({
        "=",
        ">",
        "<",
        "содержит"
    });

    ui->comboFilterBreedField->addItems({
        "наименование",
        "характеристика",
        "окрас",
        "мин высота",
        "макс высота"
    });

    connect(ui->btnFilterBreed,
            &QPushButton::clicked,
            this,
            [this]()
            {
                QString field = ui->comboFilterBreedField->currentText();
                QString condition = ui->comboFilterBreedCondition->currentText();
                QString value = ui->lineFilterBreedValue->text();

                QString filter;

                if(condition == "содержит")
                    filter = QString("\"%1\" ILIKE '%%2%'").arg(field).arg(value);
                else
                    filter = QString("\"%1\" %2 '%3'").arg(field).arg(condition).arg(value);

                breedModel->setFilter(filter);
                breedModel->select();
            });

    // ФИЛЬТР ВЫСТАВОК

    ui->comboFilterExhCondition->addItems({"=",">","<","содержит"});
    ui->comboFilterExhField->addItems({"код собаки","оценка","медаль","дата"});

    connect(ui->btnFilterExh,
            &QPushButton::clicked,
            this,
            [this]()
            {
                QString field = ui->comboFilterExhField->currentText();
                QString condition = ui->comboFilterExhCondition->currentText();
                QString value = ui->lineFilterExhValue->text();

                QString filter;

                if(condition == "содержит")
                    filter = QString("CAST(\"%1\" AS TEXT) ILIKE '%%2%'").arg(field).arg(value);
                else
                    filter = QString("\"%1\" %2 '%3'").arg(field).arg(condition).arg(value);

                exhibModel->setFilter(filter);
                exhibModel->select();
            });

    // ФИЛЬТР РОДИТЕЛЕЙ

    ui->comboFilterParentCondition->addItems({"=",">","<","содержит"});
    ui->comboFilterParentField->addItems({"код собаки","код отца","код матери"});

    connect(ui->btnFilterParent,
            &QPushButton::clicked,
            this,
            [this]()
            {
                QString field = ui->comboFilterParentField->currentText();
                QString condition = ui->comboFilterParentCondition->currentText();
                QString value = ui->lineFilterParentValue->text();

                QString filter;

                if(condition == "содержит")
                    filter = QString("CAST(\"%1\" AS TEXT) ILIKE '%%2%'").arg(field).arg(value);
                else
                    filter = QString("\"%1\" %2 '%3'").arg(field).arg(condition).arg(value);

                parentModel->setFilter(filter);
                parentModel->select();
            });

    // ФИЛЬТР ВЯЗОК

    ui->comboFilterMatingCondition->addItems({"=",">","<","содержит"});
    ui->comboFilterMatingField->addItems({"самец","самка","дата"});

    connect(ui->btnFilterMating,
            &QPushButton::clicked,
            this,
            [this]()
            {
                QString field = ui->comboFilterMatingField->currentText();
                QString condition = ui->comboFilterMatingCondition->currentText();
                QString value = ui->lineFilterMatingValue->text();

                QString filter;

                if(condition == "содержит")
                    filter = QString("CAST(\"%1\" AS TEXT) ILIKE '%%2%'").arg(field).arg(value);
                else
                    filter = QString("\"%1\" %2 '%3'").arg(field).arg(condition).arg(value);

                matingModel->setFilter(filter);
                matingModel->select();
            });

    // ФИЛЬТР БОЛЕЗНЕЙ

    ui->comboFilterDiseaseCondition->addItems({"=",">","<","содержит"});
    ui->comboFilterDiseaseField->addItems({"наименование","метод лечения"});

    connect(ui->btnFilterDisease,
            &QPushButton::clicked,
            this,
            [this]()
            {
                QString field = ui->comboFilterDiseaseField->currentText();
                QString condition = ui->comboFilterDiseaseCondition->currentText();
                QString value = ui->lineFilterDiseaseValue->text();

                QString filter;

                if(condition == "содержит")
                    filter = QString("\"%1\" ILIKE '%%2%'").arg(field).arg(value);
                else
                    filter = QString("\"%1\" %2 '%3'").arg(field).arg(condition).arg(value);

                diseaseModel->setFilter(filter);
                diseaseModel->select();
            });

    // ФИЛЬТР ИСТОРИИ

    ui->comboFilterHistoryCondition->addItems({"=",">","<","содержит"});
    ui->comboFilterHistoryField->addItems({"код собаки","код болезни","дата начала","дата окончания"});

    connect(ui->btnFilterHistory,
            &QPushButton::clicked,
            this,
            [this]()
            {
                QString field = ui->comboFilterHistoryField->currentText();
                QString condition = ui->comboFilterHistoryCondition->currentText();
                QString value = ui->lineFilterHistoryValue->text();

                QString filter;

                if(condition == "содержит")
                    filter = QString("CAST(\"%1\" AS TEXT) ILIKE '%%2%'").arg(field).arg(value);
                else
                    filter = QString("\"%1\" %2 '%3'").arg(field).arg(condition).arg(value);

                historyModel->setFilter(filter);
                historyModel->select();
            });

    // ПОРОДЫ

    connect(ui->btnAddBreed,
            &QPushButton::clicked,
            this,
            [this]()
            {
                QSqlQuery q;

                q.exec("SELECT COALESCE(MAX(\"код породы\"),0)+1 FROM \"породы\"");

                int nextId = 1;

                if(q.next())
                    nextId = q.value(0).toInt();

                int row = breedModel->rowCount();

                breedModel->insertRow(row);

                breedModel->setData(breedModel->index(row,0), nextId);
                breedModel->setData(breedModel->index(row,1), "Новая порода");
                breedModel->setData(breedModel->index(row,2), "Характеристика...");
                breedModel->setData(breedModel->index(row,3), 40);
                breedModel->setData(breedModel->index(row,4), 60);
                breedModel->setData(breedModel->index(row,5), "Окрас...");

                breedModel->submitAll();
                breedModel->select();
            });

    connect(ui->btnDeleteBreed,
            &QPushButton::clicked,
            this,
            [this]()
            {
                QModelIndexList rows =
                    ui->tableBreeds->selectionModel()->selectedRows();

                for(const QModelIndex &index : rows)
                    breedModel->removeRow(index.row());

                breedModel->submitAll();
                breedModel->select();
            });

    connect(ui->btnRefreshBreed,
            &QPushButton::clicked,
            this,
            [this]()
            {
                breedModel->select();
            });

    // ВЫСТАВКИ

    connect(ui->btnAddExh,
            &QPushButton::clicked,
            this,
            [this]()
            {
                QSqlQuery q;

                q.exec("SELECT COALESCE(MAX(\"код выставки\"),0)+1 FROM \"выставки\"");

                int nextId = 1;

                if(q.next())
                    nextId = q.value(0).toInt();

                int row = exhibModel->rowCount();

                exhibModel->insertRow(row);

                exhibModel->setData(exhibModel->index(row,0), nextId);
                exhibModel->setData(exhibModel->index(row,1), 1);
                exhibModel->setData(exhibModel->index(row,2), QDate::currentDate());
                exhibModel->setData(exhibModel->index(row,3), 5);
                exhibModel->setData(exhibModel->index(row,4), false);

                exhibModel->submitAll();
                exhibModel->select();
            });

    connect(ui->btnDeleteExh,
            &QPushButton::clicked,
            this,
            [this]()
            {
                QModelIndexList rows =
                    ui->tableExhibitions->selectionModel()->selectedRows();

                for(const QModelIndex &index : rows)
                    exhibModel->removeRow(index.row());

                exhibModel->submitAll();
                exhibModel->select();
            });

    connect(ui->btnRefreshExh,
            &QPushButton::clicked,
            this,
            [this]()
            {
                exhibModel->select();
            });

    // РОДИТЕЛИ

    connect(ui->btnAddParent,
            &QPushButton::clicked,
            this,
            [this]()
            {
                int row = parentModel->rowCount();

                parentModel->insertRow(row);

                parentModel->setData(parentModel->index(row,0), 1);
                parentModel->setData(parentModel->index(row,1), 1);
                parentModel->setData(parentModel->index(row,2), 2);

                parentModel->submitAll();
                parentModel->select();
            });

    connect(ui->btnDeleteParent,
            &QPushButton::clicked,
            this,
            [this]()
            {
                QModelIndexList rows =
                    ui->tableParents->selectionModel()->selectedRows();

                for(const QModelIndex &index : rows)
                    parentModel->removeRow(index.row());

                parentModel->submitAll();
                parentModel->select();
            });

    connect(ui->btnRefreshParent,
            &QPushButton::clicked,
            this,
            [this]()
            {
                parentModel->select();
            });

    // ВЯЗКИ

    connect(ui->btnAddMating,
            &QPushButton::clicked,
            this,
            [this]()
            {
                QSqlQuery q;

                q.exec("SELECT COALESCE(MAX(\"код вязки\"),0)+1 FROM \"вязки\"");

                int nextId = 1;

                if(q.next())
                    nextId = q.value(0).toInt();

                int row = matingModel->rowCount();

                matingModel->insertRow(row);

                matingModel->setData(matingModel->index(row,0), nextId);
                matingModel->setData(matingModel->index(row,1), 1);
                matingModel->setData(matingModel->index(row,2), 2);
                matingModel->setData(matingModel->index(row,3), QDate::currentDate());

                matingModel->submitAll();
                matingModel->select();
            });

    connect(ui->btnDeleteMating,
            &QPushButton::clicked,
            this,
            [this]()
            {
                QModelIndexList rows =
                    ui->tableMating->selectionModel()->selectedRows();

                for(const QModelIndex &index : rows)
                    matingModel->removeRow(index.row());

                matingModel->submitAll();
                matingModel->select();
            });

    connect(ui->btnRefreshMating,
            &QPushButton::clicked,
            this,
            [this]()
            {
                matingModel->select();
            });

    // БОЛЕЗНИ

    connect(ui->btnAddDisease,
            &QPushButton::clicked,
            this,
            [this]()
            {
                QSqlQuery q;

                q.exec("SELECT COALESCE(MAX(\"код болезни\"),0)+1 FROM \"болезни\"");

                int nextId = 1;

                if(q.next())
                    nextId = q.value(0).toInt();

                int row = diseaseModel->rowCount();

                diseaseModel->insertRow(row);

                diseaseModel->setData(diseaseModel->index(row,0), nextId);
                diseaseModel->setData(diseaseModel->index(row,1), "Новая болезнь");
                diseaseModel->setData(diseaseModel->index(row,2), "Лечение...");

                diseaseModel->submitAll();
                diseaseModel->select();
            });

    connect(ui->btnDeleteDisease,
            &QPushButton::clicked,
            this,
            [this]()
            {
                QModelIndexList rows =
                    ui->tableDiseases->selectionModel()->selectedRows();

                for(const QModelIndex &index : rows)
                    diseaseModel->removeRow(index.row());

                diseaseModel->submitAll();
                diseaseModel->select();
            });

    connect(ui->btnRefreshDisease,
            &QPushButton::clicked,
            this,
            [this]()
            {
                diseaseModel->select();
            });

    // ИСТОРИЯ БОЛЕЗНЕЙ

    connect(ui->btnAddHistory,
            &QPushButton::clicked,
            this,
            [this]()
            {
                QSqlQuery q;

                q.exec("SELECT COALESCE(MAX(\"код записи\"),0)+1 FROM \"история болезней\"");

                int nextId = 1;

                if(q.next())
                    nextId = q.value(0).toInt();

                int row = historyModel->rowCount();

                historyModel->insertRow(row);

                historyModel->setData(historyModel->index(row,0), nextId);
                historyModel->setData(historyModel->index(row,1), 1);
                historyModel->setData(historyModel->index(row,2), 1);
                historyModel->setData(historyModel->index(row,3), QDate::currentDate());

                historyModel->submitAll();
                historyModel->select();
            });

    connect(ui->btnDeleteHistory,
            &QPushButton::clicked,
            this,
            [this]()
            {
                QModelIndexList rows =
                    ui->tableHistory->selectionModel()->selectedRows();

                for(const QModelIndex &index : rows)
                    historyModel->removeRow(index.row());

                historyModel->submitAll();
                historyModel->select();
            });

    connect(ui->btnRefreshHistory,
            &QPushButton::clicked,
            this,
            [this]()
            {
                historyModel->select();
            });

    // СБРОС ФИЛЬТРОВ

    connect(ui->btnClearFilterDogs,
            &QPushButton::clicked,
            this,
            [this]()
            {
                ui->lineFilterDogsValue->clear();
                model->setFilter("");
                model->select();
            });

    connect(ui->btnClearFilterBreed,
            &QPushButton::clicked,
            this,
            [this]()
            {
                ui->lineFilterBreedValue->clear();
                breedModel->setFilter("");
                breedModel->select();
            });

    connect(ui->btnClearFilterExh,
            &QPushButton::clicked,
            this,
            [this]()
            {
                ui->lineFilterExhValue->clear();
                exhibModel->setFilter("");
                exhibModel->select();
            });

    connect(ui->btnClearFilterParent,
            &QPushButton::clicked,
            this,
            [this]()
            {
                ui->lineFilterParentValue->clear();
                parentModel->setFilter("");
                parentModel->select();
            });

    connect(ui->btnClearFilterMating,
            &QPushButton::clicked,
            this,
            [this]()
            {
                ui->lineFilterMatingValue->clear();
                matingModel->setFilter("");
                matingModel->select();
            });

    connect(ui->btnClearFilterDisease,
            &QPushButton::clicked,
            this,
            [this]()
            {
                ui->lineFilterDiseaseValue->clear();
                diseaseModel->setFilter("");
                diseaseModel->select();
            });

    connect(ui->btnClearFilterHistory,
            &QPushButton::clicked,
            this,
            [this]()
            {
                ui->lineFilterHistoryValue->clear();
                historyModel->setFilter("");
                historyModel->select();
            });

    // ОТЧЕТЫ

    connect(ui->btnPairs, &QPushButton::clicked, this, [this]()

            {
                bool ok;

                int minRating = QInputDialog::getInt(
                    this,
                    "Фильтр отчёта",
                    "Минимальная средняя оценка:",
                    4, 1, 5, 1, &ok);

                if(!ok)
                    return;

                QString sortField = QInputDialog::getItem(
                    this,
                    "Сортировка",
                    "Сортировать по:",
                    {
                        "средняя оценка",
                        "количество выставок",
                        "кличка самца",
                        "кличка самки"
                    },
                    0,
                    false,
                    &ok);

                if(!ok)
                    return;

                QString sortOrder = QInputDialog::getItem(
                    this,
                    "Порядок",
                    "Порядок сортировки:",
                    {
                        "по убыванию",
                        "по возрастанию"
                    },
                    0,
                    false,
                    &ok);

                if(!ok)
                    return;

                QString orderDir =
                    (sortOrder == "по убыванию")
                        ? "DESC"
                        : "ASC";

                QString orderBy;

                if(sortField == "средняя оценка")
                {
                    orderBy =
                        QString("(AVG(e1.\"оценка\") + AVG(e2.\"оценка\")) / 2 %1")
                            .arg(orderDir);
                }
                else if(sortField == "количество выставок")
                {
                    orderBy =
                        QString("COUNT(e1.\"код выставки\") + COUNT(e2.\"код выставки\") %1")
                            .arg(orderDir);
                }
                else if(sortField == "кличка самца")
                {
                    orderBy =
                        QString("d1.\"кличка\" %1")
                            .arg(orderDir);
                }
                else
                {
                    orderBy =
                        QString("d2.\"кличка\" %1")
                            .arg(orderDir);
                }

                QSqlQuery query;

                query.prepare(
                    "SELECT d1.\"кличка\", d2.\"кличка\", "
                    "AVG(e1.\"оценка\"), AVG(e2.\"оценка\"), "
                    "COUNT(e1.\"код выставки\"), COUNT(e2.\"код выставки\") "
                    "FROM \"собаки\" d1 "
                    "JOIN \"собаки\" d2 "
                    "ON d1.\"код собаки\" < d2.\"код собаки\" "
                    "JOIN \"выставки\" e1 "
                    "ON e1.\"код собаки\" = d1.\"код собаки\" "
                    "JOIN \"выставки\" e2 "
                    "ON e2.\"код собаки\" = d2.\"код собаки\" "
                    "WHERE d1.\"пол\"='M' "
                    "AND d2.\"пол\"='F' "
                    "AND NOT \"проверка_родства_для_вязки\"("
                    "d1.\"код собаки\", "
                    "d2.\"код собаки\") "
                    "GROUP BY d1.\"кличка\", d2.\"кличка\" "
                    "HAVING AVG(e1.\"оценка\") >= :rating "
                    "AND AVG(e2.\"оценка\") >= :rating "
                    "ORDER BY " + orderBy
                    );

                query.bindValue(":rating", minRating);

                query.exec();

                QString result =
                    "САМЕЦ | САМКА | СР.ОЦЕНКА | ВЫСТАВОК\n\n";

                while(query.next())
                {
                    double totalAvg =
                        (
                            query.value(2).toDouble()
                            +
                            query.value(3).toDouble()
                            ) / 2.0;

                    int totalShows =
                        query.value(4).toInt()
                        +
                        query.value(5).toInt();

                    result +=
                        query.value(0).toString()
                        + " | "
                        + query.value(1).toString()
                        + " | "
                        + QString::number(totalAvg, 'f', 2)
                        + " | "
                        + QString::number(totalShows)
                        + "\n";
                }

                QMessageBox::information(
                    this,
                    "Отчёт: пары для вязки",
                    result
                    );

            });

        // ЭЛИТНЫЕ ПАРЫ

        connect(ui->btnElite, &QPushButton::clicked, this, [this]()


                {
                    bool ok;


                        int minScore = QInputDialog::getInt(
                            this, "Фильтр отчёта", "Минимальная оценка щенков:",
                            5, 1, 5, 1, &ok);
                    if(!ok) return;

                    QString sortField = QInputDialog::getItem(
                        this, "Сортировка", "Сортировать по:",
                        {"средняя оценка щенков", "количество щенков", "количество медалей",
                         "кличка самца", "кличка самки"},
                        0, false, &ok);
                    if(!ok) return;

                    QString sortOrder = QInputDialog::getItem(
                        this, "Порядок", "Порядок сортировки:",
                        {"по убыванию", "по возрастанию"},
                        0, false, &ok);
                    if(!ok) return;

                    QString orderDir = (sortOrder == "по убыванию") ? "DESC" : "ASC";

                    QString orderBy;

                    if(sortField == "средняя оценка щенков")
                        orderBy = QString("AVG(ep.\"оценка\") %1").arg(orderDir);
                    else if(sortField == "количество щенков")
                        orderBy = QString("COUNT(DISTINCT puppy.\"код собаки\") %1").arg(orderDir);
                    else if(sortField == "количество медалей")
                        orderBy = QString("COUNT(DISTINCT e1.\"код выставки\") + COUNT(DISTINCT e2.\"код выставки\") %1").arg(orderDir);
                    else if(sortField == "кличка самца")
                        orderBy = QString("d1.\"кличка\" %1").arg(orderDir);
                    else
                        orderBy = QString("d2.\"кличка\" %1").arg(orderDir);

                    QSqlQuery query;

                    query.prepare(
                        "SELECT d1.\"кличка\", d2.\"кличка\", "
                        "COUNT(DISTINCT puppy.\"код собаки\"), AVG(ep.\"оценка\"), "
                        "COUNT(DISTINCT e1.\"код выставки\") + COUNT(DISTINCT e2.\"код выставки\") "
                        "FROM \"вязки\" m "
                        "JOIN \"собаки\" d1 ON d1.\"код собаки\" = m.\"самец\" "
                        "JOIN \"собаки\" d2 ON d2.\"код собаки\" = m.\"самка\" "
                        "JOIN \"собаки\" puppy ON puppy.\"код вязки\" = m.\"код вязки\" "
                        "JOIN \"выставки\" ep ON ep.\"код собаки\" = puppy.\"код собаки\" "
                        "LEFT JOIN \"выставки\" e1 ON e1.\"код собаки\" = d1.\"код собаки\" AND e1.\"медаль\" = true "
                        "LEFT JOIN \"выставки\" e2 ON e2.\"код собаки\" = d2.\"код собаки\" AND e2.\"медаль\" = true "
                        "GROUP BY d1.\"кличка\", d2.\"кличка\" "
                        "HAVING AVG(ep.\"оценка\") >= :score "
                        "ORDER BY " + orderBy
                        );

                    query.bindValue(":score", minScore);
                    query.exec();

                    QString result = "САМЕЦ | САМКА | ЩЕНКОВ | СР.ОЦЕНКА | МЕДАЛЕЙ\n\n";

                    while(query.next())
                    {
                        result += query.value(0).toString() + " | "
                                  + query.value(1).toString() + " | "
                                  + query.value(2).toString() + " | "
                                  + QString::number(query.value(3).toDouble(), 'f', 2) + " | "
                                  + query.value(4).toString() + "\n";
                    }

                    QMessageBox::information(this, "Элитная вязка", result);


                });

        // СЛУЖЕБНЫЕ СОБАКИ

        connect(ui->btnService, &QPushButton::clicked, this, [this]()


                {
                    bool ok;


                        int minScore = QInputDialog::getInt(
                            this, "Фильтр отчёта", "Минимальная средняя оценка:",
                            3, 1, 5, 1, &ok);
                    if(!ok) return;

                    QString sortField = QInputDialog::getItem(
                        this, "Сортировка", "Сортировать по:",
                        {"средняя оценка", "количество болезней", "количество выставок", "кличка"},
                        0, false, &ok);
                    if(!ok) return;

                    QString sortOrder = QInputDialog::getItem(
                        this, "Порядок", "Порядок сортировки:",
                        {"по убыванию", "по возрастанию"},
                        0, false, &ok);
                    if(!ok) return;

                    QString orderDir = (sortOrder == "по убыванию") ? "DESC" : "ASC";

                    QString orderBy;

                    if(sortField == "средняя оценка")
                        orderBy = QString("AVG(e.\"оценка\") %1").arg(orderDir);
                    else if(sortField == "количество болезней")
                        orderBy = QString("COUNT(DISTINCT h.\"код записи\") %1").arg(orderDir);
                    else if(sortField == "количество выставок")
                        orderBy = QString("COUNT(DISTINCT e.\"код выставки\") %1").arg(orderDir);
                    else
                        orderBy = QString("s.\"кличка\" %1").arg(orderDir);

                    QSqlQuery query;

                    query.prepare(
                        "SELECT s.\"кличка\", AVG(e.\"оценка\"), "
                        "COUNT(DISTINCT h.\"код записи\"), COUNT(DISTINCT e.\"код выставки\") "
                        "FROM \"собаки\" s "
                        "LEFT JOIN \"выставки\" e ON e.\"код собаки\" = s.\"код собаки\" "
                        "LEFT JOIN \"история болезней\" h ON h.\"код собаки\" = s.\"код собаки\" "
                        "WHERE s.\"оценка психики\" = 5 "
                        "AND NOT EXISTS ("
                        "SELECT 1 FROM \"история болезней\" hh "
                        "JOIN \"болезни\" b ON b.\"код болезни\" = hh.\"код болезни\" "
                        "WHERE hh.\"код собаки\" = s.\"код собаки\" "
                        "AND b.\"наименование\" = 'Чума'"
                        ") "
                        "GROUP BY s.\"кличка\" "
                        "HAVING AVG(e.\"оценка\") >= :score "
                        "ORDER BY " + orderBy
                        );

                    query.bindValue(":score", minScore);
                    query.exec();

                    QString result = "КЛИЧКА | СР.ОЦЕНКА | БОЛЕЗНЕЙ | ВЫСТАВОК\n\n";

                    while(query.next())
                    {
                        result += query.value(0).toString() + " | "
                                  + QString::number(query.value(1).toDouble(), 'f', 2) + " | "
                                  + query.value(2).toString() + " | "
                                  + query.value(3).toString() + "\n";
                    }

                    QMessageBox::information(this, "Служебные собаки", result);


                });
}

// ПОДКЛЮЧЕНИЕ К БД

void MainWindow::connectToDB()
{
    db = QSqlDatabase::addDatabase("QPSQL");


        db.setHostName("localhost");
    db.setPort(5432);
    db.setDatabaseName("dog_club");
    db.setUserName("postgres");
    db.setPassword("1234");

    if(!db.open())
    {
        QMessageBox::critical(this,
                              "Ошибка подключения",
                              db.lastError().text());
    }


}

// НАСТРОЙКА ТАБЛИЦ

void MainWindow::setupTable()
{
    model = new QSqlTableModel(this, db);
    model->setTable("собаки");
    model->setEditStrategy(QSqlTableModel::OnFieldChange);
    model->select();


        ui->tableDogs->setModel(model);
    ui->tableDogs->setSortingEnabled(true);
    ui->tableDogs->resizeColumnsToContents();


}

void MainWindow::setupBreeds()
{
    breedModel = new QSqlTableModel(this, db);
    breedModel->setTable("породы");
    breedModel->setEditStrategy(QSqlTableModel::OnFieldChange);
    breedModel->select();


        ui->tableBreeds->setModel(breedModel);
    ui->tableBreeds->setSortingEnabled(true);
    ui->tableBreeds->resizeColumnsToContents();


}

void MainWindow::setupExhibitions()
{
    exhibModel = new QSqlTableModel(this, db);
    exhibModel->setTable("выставки");
    exhibModel->setEditStrategy(QSqlTableModel::OnFieldChange);
    exhibModel->select();


        ui->tableExhibitions->setModel(exhibModel);
    ui->tableExhibitions->setSortingEnabled(true);
    ui->tableExhibitions->resizeColumnsToContents();


}

void MainWindow::setupParents()
{
    parentModel = new QSqlTableModel(this, db);
    parentModel->setTable("родители");
    parentModel->setEditStrategy(QSqlTableModel::OnFieldChange);
    parentModel->select();


        ui->tableParents->setModel(parentModel);
    ui->tableParents->setSortingEnabled(true);
    ui->tableParents->resizeColumnsToContents();


}

void MainWindow::setupMating()
{
    matingModel = new QSqlTableModel(this, db);
    matingModel->setTable("вязки");
    matingModel->setEditStrategy(QSqlTableModel::OnFieldChange);
    matingModel->select();


        ui->tableMating->setModel(matingModel);
    ui->tableMating->setSortingEnabled(true);
    ui->tableMating->resizeColumnsToContents();


}

void MainWindow::setupDiseaseTable()
{
    diseaseModel = new QSqlTableModel(this, db);
    diseaseModel->setTable("болезни");
    diseaseModel->setEditStrategy(QSqlTableModel::OnFieldChange);
    diseaseModel->select();


        ui->tableDiseases->setModel(diseaseModel);
    ui->tableDiseases->setSortingEnabled(true);
    ui->tableDiseases->resizeColumnsToContents();


}

void MainWindow::setupHistory()
{
    historyModel = new QSqlTableModel(this, db);
    historyModel->setTable("история болезней");
    historyModel->setEditStrategy(QSqlTableModel::OnFieldChange);
    historyModel->select();


        ui->tableHistory->setModel(historyModel);
    ui->tableHistory->setSortingEnabled(true);
    ui->tableHistory->resizeColumnsToContents();


}

void MainWindow::loadBreeds()
{
    qDebug() << "Breeds loaded";
}

// ДЕСТРУКТОР

MainWindow::~MainWindow()
{
    delete ui;
}























